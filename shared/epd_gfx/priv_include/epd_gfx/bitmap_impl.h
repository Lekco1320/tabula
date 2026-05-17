/**
 * @file bitmap_impl.h
 * @brief Definition for bitmap implementation.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2026-05-17
 * @license MIT
 */

#pragma once

#ifndef _EPD_GFX_BITMAP_IMPL_H_
#define _EPD_GFX_BITMAP_IMPL_H_

#include <stdint.h>

#include "epd_gfx/common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct epd_gfx_bitmap_impl {
    uint16_t         width;
    uint16_t         height;
    epd_gfx_format_t format;
    uint16_t         stride;
    uint32_t         buffer_bytes;
    union {
        uint8_t* buf_native;
        struct {
            uint8_t* buf_wht;
            uint8_t* buf_red;
        };
    };
};

#ifdef __cplusplus
}
#endif

#endif // !_EPD_GFX_BITMAP_IMPL_H_
