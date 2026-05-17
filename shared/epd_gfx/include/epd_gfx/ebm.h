/**
 * @file ebm.h
 * @brief EBM1 bitmap file format helpers.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2026-05-17
 * @license MIT
 */

#pragma once

#ifndef _EPD_GFX_EBM_H_
#define _EPD_GFX_EBM_H_

#include <stdbool.h>
#include <stdint.h>
#include <epd_core/stream.h>

#include "epd_gfx/common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EPD_GFX_EBM_MAGIC        "EBM1"
#define EPD_GFX_EBM_MAGIC_BYTES  4U
#define EPD_GFX_EBM_HEADER_BYTES 9U

/**
 * @brief EBM1 file header.
 */
typedef struct {
    char             magic[4];
    uint16_t         width;
    uint16_t         height;
    epd_gfx_format_t format;
} epd_gfx_ebm_header_t;

/**
 * @brief Check whether an EBM header contains the EBM1 magic.
 *
 * @param header Header to validate.
 * @return true if the magic is `EBM1`, otherwise false.
 */
bool epd_gfx_ebm_check_magic(const epd_gfx_ebm_header_t* header);

/**
 * @brief Check whether a format is valid for EBM1 bitmap data.
 *
 * @param format Bitmap format.
 * @return true if the format is supported by EBM1, otherwise false.
 */
bool epd_gfx_ebm_format_valid(epd_gfx_format_t format);

/**
 * @brief Get the total EBM1 data section size.
 *
 * @param width Bitmap width in pixels.
 * @param height Bitmap height in pixels.
 * @param format Bitmap format.
 * @return Total data bytes after the EBM1 header, or 0 if the input is invalid.
 */
uint32_t epd_gfx_ebm_data_bytes(uint16_t width, uint16_t height, epd_gfx_format_t format);

/**
 * @brief Read an EBM1 header from the current stream position.
 *
 * @param stream Source stream.
 * @param header Header buffer to fill.
 * @return true on success, otherwise false.
 */
bool epd_gfx_ebm_read_header(const epd_stream_t* stream, epd_gfx_ebm_header_t* header);

#ifdef __cplusplus
}
#endif

#endif // !_EPD_GFX_EBM_H_
