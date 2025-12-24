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
 * @brief Convert point size to pixels.
 *
 * @param point Font size in points.
 * @param dpi Target DPI.
 * @return Pixel size.
 */
uint16_t epd_gfx_font_point_to_pixel(float point, uint16_t dpi);

/**
 * @brief Convert pixel size to points.
 *
 * @param pixel Font size in pixels.
 * @param dpi DPI used for conversion.
 * @return Point size, or 0 if dpi is 0.
 */
float epd_gfx_font_pixel_to_point(uint16_t pixel, uint16_t dpi);

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
 * @brief Save a font into a custom font stream.
 *
 * @param font Font handle to serialize.
 * @param stream Stream to receive the font file data.
 * @return `EPD_OK` on success, otherwise an error code from `epd_err_t`.
 */
epd_err_t epd_gfx_font_save(const epd_gfx_font_t font, epd_stream_t* stream);

/**
 * @brief Get a glyph by codepoint from a font.
 *
 * @param font Font handle.
 * @param codepoint Unicode codepoint to query.
 * @param out_glyph Pointer to receive the glyph handle.
 * @return `EPD_OK` on success, otherwise an error code from `epd_err_t`.
 */
epd_err_t epd_gfx_font_get_glyph(const epd_gfx_font_t font, uint32_t codepoint, epd_gfx_glyph_t* out_glyph);

/**
 * @brief Add a glyph into a font.
 *
 * @param font Font handle to update.
 * @param glyph Glyph handle to add.
 * @return `EPD_OK` on success, otherwise an error code from `epd_err_t`.
 */
epd_err_t epd_gfx_font_add_glyph(epd_gfx_font_t font, const epd_gfx_glyph_t glyph);

/**
 * @brief Remove a glyph by codepoint and size.
 *
 * @param font Font handle to update.
 * @param codepoint Unicode codepoint to remove.
 * @return `EPD_OK` on success, otherwise an error code from `epd_err_t`.
 */
epd_err_t epd_gfx_font_remove_glyph(epd_gfx_font_t font, uint32_t codepoint);

/**
 * @brief Check whether a font contains a glyph for the given codepoint.
 *
 * @param font Font handle.
 * @param codepoint Unicode codepoint to query.
 * @return true if the glyph exists, otherwise false.
 */
bool epd_gfx_font_contains_glyph(const epd_gfx_font_t font, uint32_t codepoint);

#ifdef __cplusplus
}
#endif

#endif // !_EPD_GFX_FONT_H_
