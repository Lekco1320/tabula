/**
 * @file glyph.c
 * @brief Glyph data and monochrome bitmap helpers.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2025-12-22
 * @license MIT
 */

#include <stdlib.h>
#include <string.h>

#include "epd_gfx/glyph.h"
#include "epd_gfx/glyph_impl.h"

epd_err_t epd_gfx_glyph_create(const epd_gfx_glyph_config_t* config, epd_gfx_glyph_t* out_glyph)
{
    if (!config || !out_glyph) {
        return EPD_ERR_INVALID_ARG;
    }

    epd_err_t       status = EPD_OK;
    epd_gfx_glyph_t glyph  = (epd_gfx_glyph_t)calloc(1, sizeof(struct epd_gfx_glyph_impl));
    uint8_t*        data   = NULL;
    if (!glyph) {
        status = EPD_ERR_NO_MEM;
        goto fail;
    }

    glyph->width   = config->width;
    glyph->height  = config->height;
    glyph->xoffset = config->xoffset;
    glyph->yoffset = config->yoffset;
    glyph->advance = config->advance;

    if (config->size && config->data) {
        data = (uint8_t*)malloc(config->size);
        if (!data) {
            status = EPD_ERR_NO_MEM;
            goto fail;
        }
        memcpy(data, config->data, config->size);
    }

    glyph->data = data;
    *out_glyph  = glyph;
    return EPD_OK;

fail:
    if (glyph) {
        free(glyph);
    }
    if (data) {
        free(data);
    }
    return status;
}

epd_err_t epd_gfx_glyph_destroy(epd_gfx_glyph_t glyph)
{
    if (glyph) {
        return EPD_OK;
    }

    if (glyph->data) {
        free(glyph->data);
        glyph->data = NULL;
    }
    
    return EPD_OK;
}

uint16_t epd_gfx_glyph_get_width(const epd_gfx_glyph_t glyph)
{
    return (glyph ? glyph->width : 0U);
}

uint16_t epd_gfx_glyph_get_height(const epd_gfx_glyph_t glyph)
{
    return (glyph ? glyph->height : 0U);
}

int16_t epd_gfx_glyph_get_xoffset(const epd_gfx_glyph_t glyph)
{
    return (glyph ? glyph->xoffset : 0U);
}

int16_t epd_gfx_glyph_get_yoffset(const epd_gfx_glyph_t glyph)
{
    return (glyph ? glyph->yoffset : 0U);
}

int16_t epd_gfx_glyph_get_advance(const epd_gfx_glyph_t glyph)
{
    return (glyph ? glyph->advance : 0U);
}
