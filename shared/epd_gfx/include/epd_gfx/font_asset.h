/**
 * @file font_asset.h
 * @brief Editable font asset API for offline EGF font generation.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2026-04-28
 * @license MIT
 */

#pragma once

#ifndef _EPD_GFX_FONT_ASSET_H_
#define _EPD_GFX_FONT_ASSET_H_

#include <stdint.h>
#include <epd_core/common.h>
#include <epd_core/stream.h>

#include "epd_gfx/glyph.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct epd_gfx_font_asset_impl* epd_gfx_font_asset_t;

typedef struct {
    uint16_t size;
    int16_t  ascent;
    int16_t  descent;
    int16_t  line_height;
} epd_gfx_font_asset_size_config_t;

typedef struct {
    uint32_t codepoint;
    uint16_t size;
} epd_gfx_font_asset_glyph_key_t;

/**
 * @brief Create an editable font asset.
 *
 * The asset uses `name` to derive the font identity hash written into the EGF header.
 *
 * @param name Font identity string, or NULL for an anonymous asset.
 * @param out_asset Pointer to the created asset handle.
 * @return `EPD_OK` on success, otherwise an error code from `epd_err_t`.
 */
epd_err_t epd_gfx_font_asset_create(const char* name, epd_gfx_font_asset_t* out_asset);

/**
 * @brief Load an editable font asset from an EGF1 font stream.
 *
 * The asset keeps the EGF header hash; the original identity string cannot be recovered.
 *
 * @param stream Stream providing the EGF1 font data.
 * @param out_asset Pointer to the created asset handle.
 * @return `EPD_OK` on success, otherwise an error code from `epd_err_t`.
 */
epd_err_t epd_gfx_font_asset_load_egf(const epd_stream_t* stream, epd_gfx_font_asset_t* out_asset);

/**
 * @brief Destroy a font asset and release its resources.
 *
 * @param asset Font asset handle to destroy.
 * @return `EPD_OK` on success, otherwise an error code from `epd_err_t`.
 */
epd_err_t epd_gfx_font_asset_destroy(epd_gfx_font_asset_t asset);

/**
 * @brief Add or update metrics for a font size.
 *
 * @param asset Font asset handle to update.
 * @param config Size metrics configuration.
 * @return `EPD_OK` on success, otherwise an error code from `epd_err_t`.
 */
epd_err_t epd_gfx_font_asset_set_size(epd_gfx_font_asset_t asset,
    const epd_gfx_font_asset_size_config_t* config);

/**
 * @brief Remove a font size and all glyphs stored under it.
 *
 * Removing a missing size is treated as success.
 *
 * @param asset Font asset handle to update.
 * @param size Font pixel size to remove.
 * @return `EPD_OK` on success, otherwise an error code from `epd_err_t`.
 */
epd_err_t epd_gfx_font_asset_remove_size(epd_gfx_font_asset_t asset, uint16_t size);

/**
 * @brief Add or replace a glyph in a font asset.
 *
 * The asset stores its own copy of the glyph data; the caller keeps ownership of `glyph`.
 *
 * @param asset Font asset handle to update.
 * @param key Glyph key identifying where the glyph is stored.
 * @param glyph Glyph bitmap and metrics to copy into the asset.
 * @return `EPD_OK` on success, otherwise an error code from `epd_err_t`.
 */
epd_err_t epd_gfx_font_asset_add_glyph(epd_gfx_font_asset_t asset,
    epd_gfx_font_asset_glyph_key_t key, const epd_gfx_glyph_t glyph);

/**
 * @brief Remove a glyph from a font asset.
 *
 * @param asset Font asset handle to update.
 * @param key Glyph key identifying the glyph to remove.
 * @return `EPD_OK` on success, otherwise an error code from `epd_err_t`.
 */
epd_err_t epd_gfx_font_asset_remove_glyph(epd_gfx_font_asset_t asset,
    epd_gfx_font_asset_glyph_key_t key);

/**
 * @brief Check whether a font asset contains a glyph.
 *
 * @param asset Font asset handle.
 * @param key Glyph key identifying the glyph to query.
 * @return true if the glyph exists, otherwise false.
 */
bool epd_gfx_font_asset_contains_glyph(const epd_gfx_font_asset_t asset,
    epd_gfx_font_asset_glyph_key_t key);

/**
 * @brief Write a font asset as an EGF1 font stream.
 *
 * The writer serializes sizes in ascending `size` order and glyphs in ascending `codepoint` order.
 *
 * @param asset Font asset handle to serialize.
 * @param stream Writable stream to receive the EGF1 data.
 * @return `EPD_OK` on success, otherwise an error code from `epd_err_t`.
 */
epd_err_t epd_gfx_font_asset_write_egf(const epd_gfx_font_asset_t asset, epd_stream_t* stream);

#ifdef __cplusplus
}
#endif

#endif // !_EPD_GFX_FONT_ASSET_H_
