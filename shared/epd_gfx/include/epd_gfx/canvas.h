/**
 * @file canvas.h
 * @brief Canvas API for drawing into EPD buffers (native or planes).
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2025-11-21
 * @license MIT
 */

#pragma once

#ifndef _EPD_GFX_CANVAS_H_
#define _EPD_GFX_CANVAS_H_

#include <stdint.h>
#include <epd_core/common.h>

#include "epd_gfx/common.h"
#include "epd_gfx/frame_view.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct epd_gfx_canvas_impl* epd_gfx_canvas_t;

typedef struct {
    uint16_t           width;
    uint16_t           height;
    epd_gfx_format_t   format;
    epd_gfx_rotation_t rotation;
} epd_gfx_canvas_config_t;

/**
 * @brief Create a canvas buffer with the given configuration.
 *
 * @param config Canvas configuration (size, format, rotation).
 * @param out_canvas Pointer to the created canvas handle.
 * @return `EPD_OK` on success, otherwise an error code from `epd_err_t`.
 */
epd_err_t epd_gfx_canvas_create(const epd_gfx_canvas_config_t* config, epd_gfx_canvas_t* out_canvas);

/**
 * @brief Destroy a canvas and release its resources.
 *
 * @param canvas Canvas handle to destroy.
 * @return `EPD_OK` on success, otherwise an error code from `epd_err_t`.
 */
epd_err_t epd_gfx_canvas_destroy(epd_gfx_canvas_t canvas);

/**
 * @brief Get the current buffer format (native or planes).
 *
 * @param canvas Canvas handle.
 * @return The buffer format, or `EPD_GFX_FORMAT_UNKNOWN` if canvas is null.
 */
epd_gfx_format_t epd_gfx_canvas_get_format(const epd_gfx_canvas_t canvas);

/**
 * @brief Get the current rotation of the canvas.
 *
 * @param canvas Canvas handle.
 * @return The rotation, or `EPD_GFX_ROTATE_UNKNOWN` if canvas is null.
 */
epd_gfx_rotation_t epd_gfx_canvas_get_rotation(const epd_gfx_canvas_t canvas);

/**
 * @brief Set the canvas rotation and update logical dimensions/mapping.
 *
 * @param canvas Canvas handle.
 * @param rotation Target rotation.
 * @return `EPD_OK` on success, otherwise an error code from `epd_err_t`.
 */
epd_err_t epd_gfx_canvas_set_rotation(epd_gfx_canvas_t canvas, epd_gfx_rotation_t rotation);

/**
 * @brief Get the physical width (not affected by rotation).
 *
 * @param canvas Canvas handle.
 * @return Width in pixels, or 0 if canvas is null.
 */
uint16_t epd_gfx_canvas_get_width(const epd_gfx_canvas_t canvas);

/**
 * @brief Get the physical height (not affected by rotation).
 *
 * @param canvas Canvas handle.
 * @return Height in pixels, or 0 if canvas is null.
 */
uint16_t epd_gfx_canvas_get_height(const epd_gfx_canvas_t canvas);

/**
 * @brief Get the logical width (affected by rotation).
 *
 * @param canvas Canvas handle.
 * @return Logical width in pixels, or 0 if canvas is null.
 */
uint16_t epd_gfx_canvas_get_logical_width(const epd_gfx_canvas_t canvas);

/**
 * @brief Get the logical height (affected by rotation).
 *
 * @param canvas Canvas handle.
 * @return Logical height in pixels, or 0 if canvas is null.
 */
uint16_t epd_gfx_canvas_get_logical_height(const epd_gfx_canvas_t canvas);

/**
 * @brief Get a pixel at logical coordinates.
 *
 * @param canvas Canvas handle.
 * @param x Logical X coordinate.
 * @param y Logical Y coordinate.
 * @return Color of the pixel.
 */
epd_gfx_color_t epd_gfx_canvas_get_pixel(const epd_gfx_canvas_t canvas,
    uint16_t x, uint16_t y);

/**
 * @brief Clear the canvas to white.
 *
 * @param canvas Canvas handle.
 * @return `EPD_OK` on success, otherwise an error code from `epd_err_t`.
 */
epd_err_t epd_gfx_canvas_clear(epd_gfx_canvas_t canvas);

/**
 * @brief Fill the canvas with a color.
 *
 * @param canvas Canvas handle.
 * @param color Fill color.
 * @return `EPD_OK` on success, otherwise an error code from `epd_err_t`.
 */
epd_err_t epd_gfx_canvas_fill(epd_gfx_canvas_t canvas, epd_gfx_color_t color);

/**
 * @brief Draw a pixel at logical coordinates.
 *
 * @param canvas Canvas handle.
 * @param x Logical X coordinate.
 * @param y Logical Y coordinate.
 * @param color Pixel color.
 * @return `EPD_OK` on success, otherwise an error code from `epd_err_t`.
 */
epd_err_t epd_gfx_canvas_draw_pixel(epd_gfx_canvas_t canvas,
    uint16_t x, uint16_t y, epd_gfx_color_t color);

/**
 * @brief Draw a horizontal line in logical coordinates (rotation-aware).
 *
 * @param canvas Canvas handle.
 * @param x Logical start X.
 * @param y Logical start Y.
 * @param w Line length.
 * @param color Line color.
 * @return `EPD_OK` on success, otherwise an error code from `epd_err_t`.
 */
epd_err_t epd_gfx_canvas_draw_hline(epd_gfx_canvas_t canvas,
    uint16_t x, uint16_t y, uint16_t w, epd_gfx_color_t color);

/**
 * @brief Draw a vertical line in logical coordinates (rotation-aware).
 *
 * @param canvas Canvas handle.
 * @param x Logical start X.
 * @param y Logical start Y.
 * @param h Line length.
 * @param color Line color.
 * @return `EPD_OK` on success, otherwise an error code from `epd_err_t`.
 */
epd_err_t epd_gfx_canvas_draw_vline(epd_gfx_canvas_t canvas,
    uint16_t x, uint16_t y, uint16_t h, epd_gfx_color_t color);

/**
 * @brief Draw a rectangle border (four edges) in logical coordinates.
 *
 * @param canvas Canvas handle.
 * @param x Logical X of top-left corner.
 * @param y Logical Y of top-left corner.
 * @param w Rectangle width.
 * @param h Rectangle height.
 * @param color Border color.
 * @return `EPD_OK` on success, otherwise an error code from `epd_err_t`.
 */
epd_err_t epd_gfx_canvas_draw_rect(epd_gfx_canvas_t canvas,
    uint16_t x, uint16_t y, uint16_t w, uint16_t h, epd_gfx_color_t color);

/**
 * @brief Draw a filled rectangle in logical coordinates.
 *
 * @param canvas Canvas handle.
 * @param x Logical X of top-left corner.
 * @param y Logical Y of top-left corner.
 * @param w Rectangle width.
 * @param h Rectangle height.
 * @param color Fill color.
 * @return `EPD_OK` on success, otherwise an error code from `epd_err_t`.
 */
epd_err_t epd_gfx_canvas_fill_rect(epd_gfx_canvas_t canvas,
    uint16_t x, uint16_t y, uint16_t w, uint16_t h, epd_gfx_color_t color);

/**
 * @brief Present the canvas buffer to a frame sink.
 *
 * @param canvas Canvas handle.
 * @param sink Frame sink to receive the view; must not be null.
 * @return `EPD_OK` on success, otherwise an error code from `epd_err_t`.
 */
epd_err_t epd_gfx_canvas_flush(const epd_gfx_canvas_t canvas, const epd_gfx_frame_view_sink_t* sink);

/**
 * @brief Clone a canvas, including its buffer data and configuration.
 *
 * @param canvas Source canvas handle.
 * @param out_canvas Pointer to receive the cloned canvas handle.
 * @return `EPD_OK` on success, otherwise an error code from `epd_err_t`.
 */
epd_err_t epd_gfx_canvas_clone(const epd_gfx_canvas_t canvas, epd_gfx_canvas_t* out_canvas);

/**
 * @brief Load native buffer data into a native-format canvas.
 *
 * @param canvas Canvas handle (must be native format).
 * @param data Pointer to the native buffer.
 * @param size The bytes count of buffer.
 * @return `EPD_OK` on success, `EPD_ERR_INVALID_ARG` on null/format mismatch, or `EPD_ERR_INVALID_SIZE` on size mismatch.
 */
epd_err_t epd_gfx_canvas_load_native(epd_gfx_canvas_t canvas, const uint8_t* data,
    uint32_t size);

/**
 * @brief Load plane buffer data into a planes-format canvas.
 *
 * @param canvas Canvas handle (must be planes format).
 * @param pwht Pointer to the white plane buffer.
 * @param pred Pointer to the red plane buffer.
 * @param size The bytes count of buffer.
 * @return `EPD_OK` on success, `EPD_ERR_INVALID_ARG` on null/format mismatch, or `EPD_ERR_INVALID_SIZE` on size mismatch.
 */
epd_err_t epd_gfx_canvas_load_planes(epd_gfx_canvas_t canvas, const uint8_t* pwht,
    const uint8_t* pred, uint32_t size);

#ifdef __cplusplus
}
#endif

#endif // !_EPD_GFX_CANVAS_H_
