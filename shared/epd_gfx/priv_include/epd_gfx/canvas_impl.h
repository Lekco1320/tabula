/**
 * @file canvas_impl.h
 * @brief Definition for canvas implementation.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2025-12-22
 * @license MIT
 */

#pragma once

#ifndef _EPD_GFX_CANVAS_IMPL_H_
#define _EPD_GFX_CANVAS_IMPL_H_

#include <stdint.h>
#include <epd_core/common.h>

#include "epd_gfx/canvas.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*epd_gfx_map_fn_t)(const epd_gfx_canvas_t, uint16_t*, uint16_t*);
typedef uint16_t (*epd_gfx_size_fn_t)(const epd_gfx_canvas_t);

struct epd_gfx_canvas_impl {
    uint16_t          width;
    uint16_t          height;
    uint8_t           flags;      // bit0=format (0 native / 1 planes), bits1-2=rotation (0/90/180/270)
    epd_gfx_map_fn_t  map_fn;
    epd_gfx_size_fn_t lwidth_fn;
    epd_gfx_size_fn_t lheight_fn;

    uint16_t          buf_stride;
    uint32_t          buf_size;
    union {
        struct {
            uint8_t*  buf_native;
        };
        struct {
            uint8_t*  buf_wht;
            uint8_t*  buf_red;
        };
    };
};

#ifdef __cplusplus
}
#endif

#endif // !_EPD_GFX_CANVAS_IMPL_H_
