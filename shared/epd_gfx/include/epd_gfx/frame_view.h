/**
 * @file frame_view.h
 * @brief Frame view and sink interface for passing canvas buffers to outputs.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2025-12-5
 * @license MIT
 */

#pragma once

#ifndef _EPD_GFX_FRAME_VIEW_H_
#define _EPD_GFX_FRAME_VIEW_H_

#include <stdint.h>
#include <epd_core/common.h>

#include "epd_gfx/common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    epd_gfx_format_t format;
    uint16_t         width;
    uint16_t         height;
    uint16_t         stride;
    union {
        struct {
            const uint8_t* buf_native;
        };
        struct {
            const uint8_t* buf_wht;
            const uint8_t* buf_red;
        };
    };
} epd_gfx_frame_view_t;

typedef struct {
    void*     context;
    epd_err_t (*flush_impl)(void*, const epd_gfx_frame_view_t*);
} epd_gfx_frame_view_sink_t;

#ifdef __cplusplus
}
#endif

#endif // !_EPD_GFX_FRAME_VIEW_H_