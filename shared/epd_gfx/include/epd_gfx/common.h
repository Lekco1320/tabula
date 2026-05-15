/**
 * @file common.h
 * @brief Common definitions for e-paper graphics layer.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2025-11-28
 * @license MIT
 */

#pragma once

#ifndef _EPD_GFX_COMMON_H_
#define _EPD_GFX_COMMON_H_

#include <stdint.h>

typedef struct {
    uint16_t x;
    uint16_t y;
} epd_gfx_point_t;

typedef struct {
    uint16_t width;
    uint16_t height;
} epd_gfx_size_t;

typedef struct {
    uint16_t x;
    uint16_t y;
    uint16_t width;
    uint16_t height;
} epd_gfx_rect_t;

typedef enum {
    EPD_GFX_BLACK = 0b000,
    EPD_GFX_WHITE = 0b011,
    EPD_GFX_RED   = 0b100,
} epd_gfx_color_t;

typedef enum {
    EPD_GFX_BG_BLACK       = EPD_GFX_BLACK,
    EPD_GFX_BG_WHITE       = EPD_GFX_WHITE,
    EPD_GFX_BG_RED         = EPD_GFX_RED,
    EPD_GFX_BG_TRANSPARENT = 0xFF,
} epd_gfx_bg_color_t;

typedef enum {
    EPD_GFX_FORMAT_NATIVE  = 0,   // panel-native 2px/byte
    EPD_GFX_FORMAT_PLANES  = 1,   // 2 × 1bpp (black + red)
    EPD_GFX_FORMAT_UNKNOWN,
} epd_gfx_format_t;

typedef enum {
    EPD_GFX_ROTATE_0        = 0,
    EPD_GFX_ROTATE_90       = 1,
    EPD_GFX_ROTATE_180      = 2,
    EPD_GFX_ROTATE_270      = 3,
    EPD_GFX_ROTATE_UNKNOWN,
} epd_gfx_rotation_t;

#endif // !_EPD_GFX_COMMON_H_
