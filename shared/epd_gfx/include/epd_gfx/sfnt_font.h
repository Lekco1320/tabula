/**
 * @file sfnt_font.h
 * @brief SFNT (TTF/OTF) font API.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2025-12-24
 * @license MIT
 */

#pragma once

#ifndef _EPD_GFX_SFNT_FONT_H_
#define _EPD_GFX_SFNT_FONT_H_

#include <stdint.h>
#include <epd_core/common.h>

#include <epd_gfx/font_asset.h>
#include <epd_gfx/glyph.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct epd_gfx_sfnt_font_impl* epd_gfx_sfnt_font_t;

typedef struct {
    const uint8_t* data;
    uint16_t       px_size;
} epd_gfx_sfnt_font_config_t;

typedef struct {
    uint32_t codepoint;
    uint8_t  threshold;
    int8_t   bias;
#if 0
    uint8_t  bayer_order;
    uint16_t gamma_q8;  
    int8_t   embolden;
    uint8_t  oversample;
#endif
} epd_gfx_sfnt_font_render_config_t;

/**
 * @brief Create a font instance from SFNT data (TTF/OTF).
 *
 * @param config SFNT font configuration.
 * @param out_font Pointer to the created font handle.
 * @return `EPD_OK` on success, otherwise an error code from `epd_err_t`.
 */
epd_err_t epd_gfx_sfnt_font_create(const epd_gfx_sfnt_font_config_t* config, epd_gfx_sfnt_font_t* out_font);

/**
 * @brief Destroy a font and release its resources.
 *
 * @param font SFNT font handle to destroy.
 * @return `EPD_OK` on success, otherwise an error code from `epd_err_t`.
 */
epd_err_t epd_gfx_sfnt_font_destroy(epd_gfx_sfnt_font_t font);

/**
 * @brief Get size metrics from a SFNT font instance.
 *
 * @param font SFNT font handle.
 * @param size Font pixel size represented by the SFNT font instance.
 * @param out_config Pointer to receive the size metrics.
 * @return `EPD_OK` on success, otherwise an error code from `epd_err_t`.
 */
epd_err_t epd_gfx_sfnt_font_get_size_config(const epd_gfx_sfnt_font_t font,
    uint16_t size, epd_gfx_font_asset_size_config_t* out_config);

/**
 * @brief Render a glyph from a font using threshold/bias settings.
 *
 * @param font SFNT font handle.
 * @param config Glyph render configuration.
 * @param out_glyph Pointer to receive the rendered glyph handle.
 * @return `EPD_OK` on success, otherwise an error code from `epd_err_t`.
 */
epd_err_t epd_gfx_sfnt_font_render_glyph(const epd_gfx_sfnt_font_t font,
    const epd_gfx_sfnt_font_render_config_t* config, epd_gfx_glyph_t* out_glyph);

/**
 * @brief Check whether a font contains a glyph for the given codepoint.
 *
 * @param font SFNT font handle.
 * @param codepoint Unicode codepoint to query.
 * @return true if the glyph exists, otherwise false.
 */
bool epd_gfx_sfnt_font_contains_glyph(const epd_gfx_sfnt_font_t font, uint32_t codepoint);

#ifdef __cplusplus
}
#endif

#endif // !_EPD_GFX_SFNT_FONT_H_
