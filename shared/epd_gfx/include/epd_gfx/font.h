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

#include "epd_gfx/common.h"
#include "epd_gfx/glyph.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct epd_gfx_font_impl* epd_gfx_font_t;

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
 * @brief Get a glyph by codepoint from a font.
 *
 * @param font Font handle.
 * @param config Glyph seek configuration.
 * @param out_glyph Pointer to receive the glyph handle.
 * @return `EPD_OK` on success, `EPD_FALLBACK` when fallback was used, otherwise an error code from `epd_err_t`.
 */
epd_err_t epd_gfx_font_get_glyph(const epd_gfx_font_t font, epd_gfx_glyph_seek_config_t config,
    epd_gfx_glyph_t* out_glyph);

/**
 * @brief Check whether a font contains a glyph matching a seek configuration.
 *
 * @param font Font handle.
 * @param config Glyph seek configuration; fallback is applied like `epd_gfx_font_get_glyph`.
 * @return true if the glyph exists, otherwise false.
 */
bool epd_gfx_font_contains_glyph(const epd_gfx_font_t font, epd_gfx_glyph_seek_config_t config);

#ifdef __cplusplus
}
#endif

#endif // !_EPD_GFX_FONT_H_
