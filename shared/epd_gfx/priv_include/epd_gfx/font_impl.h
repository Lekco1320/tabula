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

#include <epd_core/stream.h>
#include <epd_gfx/egf.h>

#ifdef __cplusplus
extern "C" {
#endif

struct epd_gfx_font_impl {
    epd_gfx_egf_header_t header;
    epd_stream_t         stream;

    uint32_t glyph_index_table_offset;
    uint32_t glyph_data_table_offset;
};

#ifdef __cplusplus
}
#endif

#endif // !_EPD_GFX_FONT_IMPL_H_
