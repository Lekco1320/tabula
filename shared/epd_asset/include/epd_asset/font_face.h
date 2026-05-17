/**
 * @file font_face.h
 * @brief Font face API for source font rendering.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2025-12-24
 * @license MIT
 */

#pragma once

#ifndef _EPD_ASSET_FONT_FACE_H_
#define _EPD_ASSET_FONT_FACE_H_

#include <stddef.h>
#include <stdint.h>
#include <epd_core/common.h>
#include <epd_asset/font_asset.h>
#include <epd_gfx/glyph.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct epd_asset_font_face_impl* epd_asset_font_face_t;

typedef struct {
    const uint8_t* data;
    size_t         data_size;
    uint16_t       px_size;
} epd_asset_font_face_config_t;

typedef enum {
    EPD_ASSET_FONT_FACE_RENDER_MONO = 0,
    EPD_ASSET_FONT_FACE_RENDER_GRAY_THRESHOLD,
} epd_asset_font_face_render_mode_t;

typedef struct {
    uint32_t                          codepoint;
    epd_asset_font_face_render_mode_t mode;
    uint8_t                           threshold;
    int8_t                            bias;
} epd_asset_font_face_render_config_t;

/**
 * @brief Create a font face from source font data.
 *
 * @param config Source font configuration. `data` must stay valid for the font face lifetime.
 * @param out_font Pointer to the created font handle.
 * @return `EPD_OK` on success, otherwise an error code from `epd_err_t`.
 */
epd_err_t epd_asset_font_face_create(const epd_asset_font_face_config_t* config, epd_asset_font_face_t* out_font);

/**
 * @brief Destroy a font and release its resources.
 *
 * @param font Font face handle to destroy.
 * @return `EPD_OK` on success, otherwise an error code from `epd_err_t`.
 */
epd_err_t epd_asset_font_face_destroy(epd_asset_font_face_t font);

/**
 * @brief Get size info from a font face.
 *
 * @param font Font face handle.
 * @param size Font pixel size represented by the font face.
 * @param out_info Pointer to receive the size info.
 * @return `EPD_OK` on success, otherwise an error code from `epd_err_t`.
 */
epd_err_t epd_asset_font_face_get_size_info(const epd_asset_font_face_t font,
    uint16_t size, epd_asset_font_asset_size_info_t* out_info);

/**
 * @brief Render a glyph from a font face into EGF-compatible 1bpp bitmap data.
 *
 * @param font Font face handle.
 * @param config Glyph render configuration.
 * @param out_glyph Pointer to receive the rendered glyph handle. The caller owns
 * the glyph and must release it with `epd_gfx_glyph_destroy`.
 * @return `EPD_OK` on success, otherwise an error code from `epd_err_t`.
 */
epd_err_t epd_asset_font_face_render_glyph(const epd_asset_font_face_t font,
    const epd_asset_font_face_render_config_t* config, epd_gfx_glyph_t* out_glyph);

/**
 * @brief Check whether a font contains a glyph for the given codepoint.
 *
 * @param font Font face handle.
 * @param codepoint Unicode codepoint to query.
 * @return true if the glyph exists, otherwise false.
 */
bool epd_asset_font_face_contains_glyph(const epd_asset_font_face_t font, uint32_t codepoint);

#ifdef __cplusplus
}
#endif

#endif // !_EPD_ASSET_FONT_FACE_H_
