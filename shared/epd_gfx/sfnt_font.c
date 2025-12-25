/**
 * @file sfnt_font.c
 * @brief SFNT (TTF/OTF) font API.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2025-12-24
 * @license MIT
 */

#define STB_TRUETYPE_IMPLEMENTATION 1

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <xxhash.h>
#include <epd_core/math.h>
#include <stb_truetype.h>

#include "epd_gfx/font_impl.h"
#include "epd_gfx/sfnt_font.h"

struct epd_gfx_sfnt_font_impl {
    uint64_t       hash;
    const uint8_t* data;
    stbtt_fontinfo info;

    float          scale;
    int16_t        ascent;
    int16_t        descent;
    int16_t        line_height;
};

epd_err_t epd_gfx_sfnt_font_create(const epd_gfx_sfnt_font_config_t* config, epd_gfx_sfnt_font_t* out_font)
{
    if (!config || !out_font || !config->data || config->px_size == 0U) {
        return EPD_ERR_INVALID_ARG;
    }

    epd_err_t ret = EPD_OK;
    epd_gfx_sfnt_font_t font = (epd_gfx_sfnt_font_t)calloc(1, sizeof(struct epd_gfx_sfnt_font_impl));
    if (!font) {
        return EPD_ERR_NO_MEM;
    }

    const char* sutf8 = config->name;
    font->hash = config->name ? XXH3_64bits(sutf8, strlen(sutf8)) : 0UL;
    font->data = config->data;
    if (!stbtt_InitFont(&font->info, (const unsigned char*)font->data, 0)) {
        ret = EPD_FAIL;
        goto fail;
    }

    int ascent = 0;
    int descent = 0;
    int line_gap = 0;
    font->scale = stbtt_ScaleForPixelHeight(&font->info, config->px_size);
    stbtt_GetFontVMetrics(&font->info, &ascent, &descent, &line_gap);

    font->ascent      = (int16_t)((float)ascent * font->scale);
    font->descent     = (int16_t)((float)descent * font->scale);
    font->line_height = (int16_t)((float)(ascent - descent + line_gap) * font->scale);

    *out_font = font;
    return EPD_OK;

fail:
    free(font);
    return ret;
}

epd_err_t epd_gfx_sfnt_font_destroy(epd_gfx_sfnt_font_t font)
{
    if (font) {
        free(font);
    }

    return EPD_OK;
}

epd_err_t epd_gfx_sfnt_font_generate_font(const epd_gfx_sfnt_font_t font, epd_gfx_font_t* out_font)
{
    if (!font || !out_font) {
        return EPD_ERR_INVALID_ARG;
    }

    epd_gfx_font_t gfx_font = (epd_gfx_font_t)calloc(1, sizeof(struct epd_gfx_font_impl));
    if (!gfx_font) {
        return EPD_ERR_NO_MEM;
    }

    gfx_font->hash        = font->hash;
    gfx_font->data        = NULL;
    gfx_font->scale       = font->scale;
    gfx_font->ascent      = font->ascent;
    gfx_font->descent     = font->descent;
    gfx_font->line_height = font->line_height;

    *out_font = gfx_font;
    return EPD_OK;
}

epd_err_t epd_gfx_sfnt_font_render_glyph(const epd_gfx_sfnt_font_t font, const epd_gfx_sfnt_font_render_config_t* config,
    epd_gfx_glyph_t* out_glyph)
{
    if (!font || !config || !out_glyph) {
        return EPD_ERR_INVALID_ARG;
    }

    if (!stbtt_FindGlyphIndex(&font->info, config->codepoint)) {
        return EPD_ERR_NOT_FOUND;
    }

    epd_gfx_glyph_t glyph = (epd_gfx_glyph_t)calloc(1, sizeof(struct epd_gfx_glyph_impl));
    if (!glyph) {
        return EPD_ERR_NO_MEM;
    }

    // Get gray scale bitmap
    int w = 0, h = 0, xoff = 0, yoff = 0;
    uint8_t* bmp = (uint8_t*)stbtt_GetCodepointBitmap(&font->info, 0.0f, font->scale, config->codepoint,
        &w, &h, &xoff, &yoff);

    // Get advance and left side bearing
    int adv_units = 0, lsb_units = 0;
    stbtt_GetCodepointHMetrics(&font->info, config->codepoint, &adv_units, &lsb_units);

    // Set metrics
    glyph->width       = (uint16_t)w;
    glyph->height      = (uint16_t)h;
    glyph->xoffset     = (int16_t)xoff;
    glyph->yoffset     = (int16_t)yoff;
    glyph->advance     = (int16_t)lroundf((float)adv_units * font->scale);
    glyph->ascent      = font->ascent;
    glyph->line_height = font->line_height;

    // Handle empty or null bitmap
    if (w <= 0 || h <= 0 || bmp == NULL) {
        glyph->width  = 0;
        glyph->height = 0;
        glyph->data   = NULL;
        if (bmp) {
            stbtt_FreeBitmap(bmp, NULL);
        }
        *out_glyph  = glyph;
        return EPD_OK;
    }

    epd_err_t      status = EPD_OK;
    const uint16_t stride = (uint16_t)((glyph->width + 7U) / 8U);
    const uint32_t size   = (uint32_t)stride * glyph->height;
    uint8_t*       out    = (uint8_t*)calloc(1, size);
    if (!out) {
        status = EPD_ERR_NO_MEM;
        goto fail;
    }

    for (uint16_t y = 0; y < glyph->height; ++y) {
        const uint8_t* pbitmap = bmp + (uint32_t)y * glyph->width;
        uint8_t*       pout    = out + (uint32_t)y * stride;
        for (uint16_t x = 0; x < glyph->width; ++x) {
            int16_t add = (int16_t)pbitmap[x] + config->bias;
            uint8_t res = (uint8_t)EPD_MIN(255U, EPD_MAX(0U, add));
            if (res >= config->threshold) {
                uint16_t index = x / 8U;
                pout[index] |= (uint8_t)(0x80U >> (x & 7U));
            }
        }
    }

    stbtt_FreeBitmap(bmp, NULL);
    glyph->data = out;
    *out_glyph  = glyph;

    return EPD_OK;

fail:
    if (glyph) {
        free(glyph);
    }
    if (out) {
        free(out);
    }
    if (bmp) {
        stbtt_FreeBitmap(bmp, NULL);
    }

    return status;
}

bool epd_gfx_sfnt_font_contains_glyph(const epd_gfx_sfnt_font_t font, uint32_t codepoint)
{
    if (!font) {
        return false;
    }

    return stbtt_FindGlyphIndex(&font->info, codepoint);
}
