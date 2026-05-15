/**
 * @file font.h
 * @brief Custom font API and storage.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2025-12-22
 * @license MIT
 */

#pragma once

#ifndef _EPD_GFX_FONT_H_
#define _EPD_GFX_FONT_H_

#include <stdint.h>
#include <epd_core/common.h>
#include <epd_core/stream.h>

#include "epd_gfx/glyph.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct epd_gfx_font_impl* epd_gfx_font_t;

typedef struct {
    uint16_t size;
    int16_t  ascent;
    int16_t  descent;
    int16_t  line_height;
    uint32_t glyph_count;
} epd_gfx_font_size_info_t;

/**
 * @brief Load a font from a custom font stream.
 *
 * @param stream Stream providing the font file data.
 * @param out_font Pointer to the created font handle.
 * @return `EPD_OK` on success, otherwise an error code from `epd_err_t`.
 */
epd_err_t epd_gfx_font_load(const epd_stream_t* stream, epd_gfx_font_t* out_font);

/**
 * @brief Destroy a font and release its resources.
 *
 * @param font Font handle to destroy.
 * @return `EPD_OK` on success, otherwise an error code from `epd_err_t`.
 */
epd_err_t epd_gfx_font_destroy(epd_gfx_font_t font);

/**
 * @brief Get font size metrics.
 *
 * @param font Font handle.
 * @param size Font pixel size to query exactly.
 * @param out_info Pointer to receive the size info.
 * @return `EPD_OK` on success, `EPD_ERR_NOT_FOUND` if no exact size exists,
 * otherwise an error code from `epd_err_t`.
 */
epd_err_t epd_gfx_font_get_size_info(const epd_gfx_font_t font,
    uint16_t size, epd_gfx_font_size_info_t* out_info);

/**
 * @brief Check whether a font contains an exact size.
 *
 * @param font Font handle.
 * @param size Font pixel size to query exactly.
 * @return true if the size exists, otherwise false.
 */
bool epd_gfx_font_contains_size(const epd_gfx_font_t font, uint16_t size);

/**
 * @brief Get an exact glyph by size and codepoint from a font.
 *
 * @param font Font handle.
 * @param key Glyph key.
 * @param out_glyph Pointer to receive the glyph handle.
 * @return `EPD_OK` on success, `EPD_ERR_NOT_FOUND` if no exact glyph exists,
 * otherwise an error code from `epd_err_t`.
 */
epd_err_t epd_gfx_font_get_glyph(const epd_gfx_font_t font, epd_gfx_glyph_key_t key,
    epd_gfx_glyph_t* out_glyph);

/**
 * @brief Check whether a font contains an exact glyph matching a glyph key.
 *
 * @param font Font handle.
 * @param key Glyph key.
 * @return true if the glyph exists, otherwise false.
 */
bool epd_gfx_font_contains_glyph(const epd_gfx_font_t font, epd_gfx_glyph_key_t key);

#ifdef __cplusplus
}
#endif

#endif // !_EPD_GFX_FONT_H_
