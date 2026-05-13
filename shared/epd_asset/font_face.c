/**
 * @file font_face.c
 * @brief Font face API for source font rendering.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2025-12-24
 * @license MIT
 */

#include <stdlib.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <epd_core/math.h>
#include <epd_gfx/glyph.h>

#include "epd_asset/font_face.h"

struct epd_asset_font_face_impl {
    FT_Library library;
    FT_Face    face;

    uint16_t   px_size;
    int16_t    ascent;
    int16_t    descent;
    int16_t    line_height;
};

static int16_t epd_asset_font_face_ft26_6_to_i16(FT_Pos value)
{
    return (int16_t)(value >> 6);
}

static epd_err_t epd_asset_font_face_create_empty_glyph(const FT_GlyphSlot slot, epd_gfx_glyph_t* out_glyph)
{
    epd_gfx_glyph_config_t glyph_config;
    glyph_config.width   = 0U;
    glyph_config.height  = 0U;
    glyph_config.xoffset = (int16_t)slot->bitmap_left;
    glyph_config.yoffset = (int16_t)-slot->bitmap_top;
    glyph_config.advance = epd_asset_font_face_ft26_6_to_i16(slot->advance.x);
    glyph_config.data    = NULL;

    return epd_gfx_glyph_create(&glyph_config, out_glyph);
}

static epd_err_t epd_asset_font_face_render_bitmap(const FT_Bitmap* bitmap,
    uint8_t** out_data)
{
    const uint16_t width  = (uint16_t)bitmap->width;
    const uint16_t height = (uint16_t)bitmap->rows;
    const uint16_t stride = (uint16_t)epd_gfx_glyph_stride(width);
    const uint32_t size   = epd_gfx_glyph_data_bytes(width, height);
    uint8_t*       data   = (uint8_t*)calloc(1, size);
    if (!data) {
        return EPD_ERR_NO_MEM;
    }

    int32_t pitch     = bitmap->pitch;
    bool    bottom_up = false;
    if (pitch < 0) {
        pitch     = -pitch;
        bottom_up = true;
    }

    for (uint16_t y = 0U; y < height; ++y) {
        uint16_t row        = bottom_up ? (uint16_t)(height - y - 1U) : y;
        const uint8_t* src = bitmap->buffer + (uint32_t)row * (uint32_t)pitch;
        uint8_t*       dst = data + (uint32_t)y * stride;
        for (uint16_t x = 0U; x < width; ++x) {
            uint16_t src_index = x / 8U;
            uint8_t  src_mask  = (uint8_t)(0x80U >> (x & 7U));
            if (src[src_index] & src_mask) {
                uint16_t dst_index = x / 8U;
                dst[dst_index] |= (uint8_t)(0x80U >> (x & 7U));
            }
        }
    }

    *out_data = data;
    return EPD_OK;
}

epd_err_t epd_asset_font_face_create(const epd_asset_font_face_config_t* config, epd_asset_font_face_t* out_font)
{
    if (!config || !out_font || !config->data || config->data_size == 0U || config->px_size == 0U) {
        return EPD_ERR_INVALID_ARG;
    }
    *out_font = NULL;

    epd_err_t ret = EPD_OK;
    epd_asset_font_face_t font = (epd_asset_font_face_t)calloc(1, sizeof(struct epd_asset_font_face_impl));
    if (!font) {
        return EPD_ERR_NO_MEM;
    }

    if (FT_Init_FreeType(&font->library) != 0) {
        ret = EPD_FAIL;
        goto fail;
    }
    if (FT_New_Memory_Face(font->library, config->data, (FT_Long)config->data_size, 0, &font->face) != 0) {
        ret = EPD_FAIL;
        goto fail;
    }
    if (FT_Set_Pixel_Sizes(font->face, 0, config->px_size) != 0) {
        ret = EPD_FAIL;
        goto fail;
    }

    font->px_size     = config->px_size;
    font->ascent      = epd_asset_font_face_ft26_6_to_i16(font->face->size->metrics.ascender);
    font->descent     = epd_asset_font_face_ft26_6_to_i16(font->face->size->metrics.descender);
    font->line_height = epd_asset_font_face_ft26_6_to_i16(font->face->size->metrics.height);

    *out_font = font;
    return EPD_OK;

fail:
    epd_asset_font_face_destroy(font);
    return ret;
}

epd_err_t epd_asset_font_face_destroy(epd_asset_font_face_t font)
{
    if (!font) {
        return EPD_OK;
    }

    if (font->face) {
        FT_Done_Face(font->face);
        font->face = NULL;
    }
    if (font->library) {
        FT_Done_FreeType(font->library);
        font->library = NULL;
    }
    free(font);

    return EPD_OK;
}

epd_err_t epd_asset_font_face_get_size_config(const epd_asset_font_face_t font,
    uint16_t size, epd_asset_font_asset_size_config_t* out_config)
{
    if (!font || size == 0U || !out_config) {
        return EPD_ERR_INVALID_ARG;
    }

    out_config->size        = size;
    out_config->ascent      = font->ascent;
    out_config->descent     = font->descent;
    out_config->line_height = font->line_height;
    return EPD_OK;
}

epd_err_t epd_asset_font_face_render_glyph(const epd_asset_font_face_t font,
    const epd_asset_font_face_render_config_t* config, epd_gfx_glyph_t* out_glyph)
{
    if (!font || !config || !out_glyph) {
        return EPD_ERR_INVALID_ARG;
    }

    if (FT_Get_Char_Index(font->face, config->codepoint) == 0U) {
        return EPD_ERR_NOT_FOUND;
    }
    if (FT_Load_Char(font->face, config->codepoint, FT_LOAD_RENDER | FT_LOAD_TARGET_MONO) != 0) {
        return EPD_FAIL;
    }

    FT_GlyphSlot slot = font->face->glyph;
    FT_Bitmap*   bmp  = &slot->bitmap;
    if (bmp->width == 0U || bmp->rows == 0U || !bmp->buffer) {
        return epd_asset_font_face_create_empty_glyph(slot, out_glyph);
    }
    if (bmp->pixel_mode != FT_PIXEL_MODE_MONO) {
        return EPD_ERR_NOT_SUPPORTED;
    }

    uint8_t*  data = NULL;
    epd_err_t ret  = epd_asset_font_face_render_bitmap(bmp, &data);
    if (ret != EPD_OK) {
        return ret;
    }

    epd_gfx_glyph_config_t glyph_config;
    glyph_config.width   = (uint16_t)bmp->width;
    glyph_config.height  = (uint16_t)bmp->rows;
    glyph_config.xoffset = (int16_t)slot->bitmap_left;
    glyph_config.yoffset = (int16_t)-slot->bitmap_top;
    glyph_config.advance = epd_asset_font_face_ft26_6_to_i16(slot->advance.x);
    glyph_config.data    = data;

    ret = epd_gfx_glyph_create(&glyph_config, out_glyph);
    free(data);

    return ret;
}

bool epd_asset_font_face_contains_glyph(const epd_asset_font_face_t font, uint32_t codepoint)
{
    if (!font) {
        return false;
    }

    return FT_Get_Char_Index(font->face, codepoint) != 0U;
}
