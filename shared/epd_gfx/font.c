/**
 * @file font.c
 * @brief Custom font API and storage.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2025-12-22
 * @license MIT
 */

#include <stdlib.h>

#include "epd_gfx/font.h"
#include "epd_gfx/font_impl.h"

uint16_t epd_gfx_font_point_to_pixel(float point, uint16_t dpi)
{
    return point * (dpi / 72.0f);
}

float epd_gfx_font_pixel_to_point(uint16_t pixel, uint16_t dpi)
{
    return (dpi == 0 ? 0.0f : pixel * 72.0f / dpi);
}

epd_err_t epd_gfx_font_destroy(epd_gfx_font_t font)
{
    if (font) {
        free(font);
    }

    return EPD_OK;
}
