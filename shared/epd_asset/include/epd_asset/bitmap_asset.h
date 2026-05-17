/**
 * @file bitmap_asset.h
 * @brief Bitmap asset writer API for offline EBM generation.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2026-05-17
 * @license MIT
 */

#pragma once

#ifndef _EPD_ASSET_BITMAP_ASSET_H_
#define _EPD_ASSET_BITMAP_ASSET_H_

#include <epd_core/common.h>
#include <epd_core/stream.h>
#include <epd_gfx/frame_view.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Write a frame view as an EBM1 bitmap stream.
 *
 * The writer requires a tightly packed standard stride for the frame format.
 *
 * @param view Source frame view.
 * @param stream Writable stream to receive the EBM1 data.
 * @return `EPD_OK` on success, otherwise an error code from `epd_err_t`.
 */
epd_err_t epd_asset_bitmap_write_ebm(const epd_gfx_frame_view_t* view,
    epd_stream_t* stream);

#ifdef __cplusplus
}
#endif

#endif // !_EPD_ASSET_BITMAP_ASSET_H_
