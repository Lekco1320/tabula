/**
 * @file font_impl.h
 * @brief Definition for font implementation.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2025-12-22
 * @license MIT
 */

#pragma once

#ifndef _EPD_GFX_FONT_IMPL_H_
#define _EPD_GFX_FONT_IMPL_H_

#include <stdint.h>


#include "epd_gfx/glyph_impl.h"

#ifdef __cplusplus
extern "C" {
#endif

struct epd_gfx_font_impl {
    uint64_t       hash;
    const uint8_t* data;

    float          scale;
    int16_t        ascent;
    int16_t        descent;
    int16_t        line_height;
};

#ifdef __cplusplus
}
#endif

#endif // !_EPD_GFX_FONT_IMPL_H_
