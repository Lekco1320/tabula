/**
 * @file font_asset_impl.h
 * @brief Definition for editable font asset implementation.
 *
 * @author Luka脽 Zhang <lekco_1320@qq.com>
 * @date 2026-04-28
 * @license MIT
 */

#pragma once

#ifndef _EPD_GFX_FONT_ASSET_IMPL_H_
#define _EPD_GFX_FONT_ASSET_IMPL_H_

#include <stdint.h>

#include "epd_gfx/font_asset.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t codepoint;
    uint16_t width;
    uint16_t height;
    int16_t  xoffset;
    int16_t  yoffset;
    int16_t  advance;
    uint8_t* data;
} epd_gfx_font_asset_glyph_t;

struct epd_gfx_font_asset_glyph_node {
    epd_gfx_font_asset_glyph_t            value;
    struct epd_gfx_font_asset_glyph_node* next;
};

typedef struct epd_gfx_font_asset_glyph_node* epd_gfx_font_asset_glyph_node_t;

typedef struct {
    uint16_t size;
    int16_t  ascent;
    int16_t  descent;
    int16_t  line_height;

    epd_gfx_font_asset_glyph_node_t glyph_list;
} epd_gfx_font_asset_size_t;

struct epd_gfx_font_asset_size_node {
    epd_gfx_font_asset_size_t            value;
    struct epd_gfx_font_asset_size_node* next;
};

typedef struct epd_gfx_font_asset_size_node* epd_gfx_font_asset_size_node_t;

struct epd_gfx_font_asset_impl {
    uint64_t hash;

    epd_gfx_font_asset_size_node_t size_list;
};

#ifdef __cplusplus
}
#endif

#endif // !_EPD_GFX_FONT_ASSET_IMPL_H_
