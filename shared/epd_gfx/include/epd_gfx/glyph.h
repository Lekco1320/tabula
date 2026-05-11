/**
 * @file glyph.h
 * @brief Glyph data and monochrome bitmap helpers.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2025-12-22
 * @license MIT
 */

#pragma once

#ifndef _EPD_GFX_GLYPH_H_
#define _EPD_GFX_GLYPH_H_

#include <stdint.h>
#include <epd_core/common.h>

#include "epd_gfx/common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct epd_gfx_glyph_impl* epd_gfx_glyph_t;

typedef struct {
    uint16_t width;
    uint16_t height;
    int16_t  xoffset;
    int16_t  yoffset;
    int16_t  advance;
    uint8_t* data;
} epd_gfx_glyph_config_t;

typedef enum {
    EPD_GFX_GLYPH_FALLBACK_NONE    = 0,
    EPD_GFX_GLYPH_FALLBACK_SMALLER = 1,
    EPD_GFX_GLYPH_FALLBACK_LARGER  = 2,
} epd_gfx_glyph_fallback_t;

typedef struct {
    uint32_t                 codepoint;  /*!< Unicode codepoint to query. */
    uint16_t                 size;       /*!< Font pixel size to query; must be non-zero. */
    epd_gfx_glyph_fallback_t fallback;   /*!< Fallback strategy when the exact size or glyph is unavailable. */
} epd_gfx_glyph_seek_config_t;

/**
 * @brief Create a glyph with the given configuration.
 *
 * @param config Glyph configuration (metrics and bitmap data).
 * @param out_glyph Pointer to the created glyph handle.
 * @return `EPD_OK` on success, otherwise an error code from `epd_err_t`.
 */
epd_err_t epd_gfx_glyph_create(const epd_gfx_glyph_config_t* config, epd_gfx_glyph_t* out_glyph);

/**
 * @brief Destroy a glyph and release its resources.
 *
 * @param glyph Glyph handle to destroy.
 * @return `EPD_OK` on success, otherwise an error code from `epd_err_t`.
 */
epd_err_t epd_gfx_glyph_destroy(epd_gfx_glyph_t glyph);

/**
 * @brief Get glyph bitmap width.
 *
 * @param glyph Glyph handle.
 * @return Width in pixels, or 0 if glyph is null.
 */
uint16_t epd_gfx_glyph_get_width(const epd_gfx_glyph_t glyph);

/**
 * @brief Get glyph bitmap height.
 *
 * @param glyph Glyph handle.
 * @return Height in pixels, or 0 if glyph is null.
 */
uint16_t epd_gfx_glyph_get_height(const epd_gfx_glyph_t glyph);

/**
 * @brief Get glyph X offset.
 *
 * @param glyph Glyph handle.
 * @return X offset, or 0 if glyph is null.
 */
int16_t epd_gfx_glyph_get_xoffset(const epd_gfx_glyph_t glyph);

/**
 * @brief Get glyph Y offset.
 *
 * @param glyph Glyph handle.
 * @return Y offset, or 0 if glyph is null.
 */
int16_t epd_gfx_glyph_get_yoffset(const epd_gfx_glyph_t glyph);

/**
 * @brief Get glyph advance width.
 *
 * @param glyph Glyph handle.
 * @return Advance width, or 0 if glyph is null.
 */
int16_t epd_gfx_glyph_get_advance(const epd_gfx_glyph_t glyph);

#ifdef __cplusplus
}
#endif

#endif // !_EPD_GFX_GLYPH_H_
