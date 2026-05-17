/**
 * @file bitmap.h
 * @brief Custom bitmap API and storage.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2026-05-17
 * @license MIT
 */

#pragma once

#ifndef _EPD_GFX_BITMAP_H_
#define _EPD_GFX_BITMAP_H_

#include <stdint.h>
#include <epd_core/common.h>
#include <epd_core/stream.h>

#include "epd_gfx/common.h"
#include "epd_gfx/frame_view.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct epd_gfx_bitmap_impl* epd_gfx_bitmap_t;

/**
 * @brief Load a bitmap from an EBM1 stream.
 *
 * @param stream Stream providing the EBM1 bitmap data.
 * @param out_bitmap Pointer to the created bitmap handle.
 * @return `EPD_OK` on success, otherwise an error code from `epd_err_t`.
 */
epd_err_t epd_gfx_bitmap_load(const epd_stream_t* stream, epd_gfx_bitmap_t* out_bitmap);

/**
 * @brief Destroy a bitmap and release its resources.
 *
 * @param bitmap Bitmap handle to destroy.
 * @return `EPD_OK` on success, otherwise an error code from `epd_err_t`.
 */
epd_err_t epd_gfx_bitmap_destroy(epd_gfx_bitmap_t bitmap);

/**
 * @brief Get the bitmap width.
 *
 * @param bitmap Bitmap handle.
 * @return Width in pixels, or 0 if bitmap is null.
 */
uint16_t epd_gfx_bitmap_get_width(const epd_gfx_bitmap_t bitmap);

/**
 * @brief Get the bitmap height.
 *
 * @param bitmap Bitmap handle.
 * @return Height in pixels, or 0 if bitmap is null.
 */
uint16_t epd_gfx_bitmap_get_height(const epd_gfx_bitmap_t bitmap);

/**
 * @brief Get the bitmap format.
 *
 * @param bitmap Bitmap handle.
 * @return Bitmap format, or `EPD_GFX_FORMAT_UNKNOWN` if bitmap is null.
 */
epd_gfx_format_t epd_gfx_bitmap_get_format(const epd_gfx_bitmap_t bitmap);

/**
 * @brief Get a read-only frame view of bitmap data.
 *
 * The returned view references bitmap-owned buffers and remains valid only until
 * the bitmap is destroyed.
 *
 * @param bitmap Bitmap handle.
 * @param out_view Pointer to receive the frame view.
 * @return `EPD_OK` on success, otherwise an error code from `epd_err_t`.
 */
epd_err_t epd_gfx_bitmap_get_frame_view(const epd_gfx_bitmap_t bitmap,
    epd_gfx_frame_view_t* out_view);

#ifdef __cplusplus
}
#endif

#endif // !_EPD_GFX_BITMAP_H_
