/**
 * @file glyph_impl.h
 * @brief Definition for glyph implementation.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2025-12-23
 * @license MIT
 */

#pragma once

#ifndef _EPD_GFX_GLYPH_IMPL_H_
#define _EPD_GFX_GLYPH_IMPL_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct epd_gfx_glyph_impl {
    uint16_t width;
    uint16_t height;
    int16_t  xoffset;
    int16_t  yoffset;
    int16_t  advance;
    uint8_t* data;

    int16_t  ascent;
    int16_t  line_height;
};

static inline uint32_t epd_gfx_glyph_stride(uint16_t width)
{
    return (width + 7U) / 8U;
}

static inline uint32_t epd_gfx_glyph_data_bytes(uint16_t width, uint16_t height)
{
    return epd_gfx_glyph_stride(width) * height;
}

#ifdef __cplusplus
}
#endif

#endif // !_EPD_GFX_GLYPH_IMPL_H_
