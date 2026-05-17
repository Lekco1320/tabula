/**
 * @file text.h
 * @brief API for measuring and rendering text into EPD canvas.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2026-05-15
 * @license MIT
 */

#pragma once

#ifndef _EPD_GFX_TEXT_H_
#define _EPD_GFX_TEXT_H_

#include <stdbool.h>
#include <stdint.h>
#include <epd_core/common.h>

#include "epd_gfx/canvas.h"
#include "epd_gfx/common.h"
#include "epd_gfx/font.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    EPD_GFX_TEXT_ALIGN_START  = 0,
    EPD_GFX_TEXT_ALIGN_CENTER = 1,
    EPD_GFX_TEXT_ALIGN_END    = 2,
} epd_gfx_text_align_t;

typedef struct {
    uint16_t           size;
    epd_gfx_color_t    color;
    epd_gfx_bg_color_t background;
    int16_t            letter_spacing;
} epd_gfx_text_style_t;

typedef struct {
    epd_gfx_text_style_t text;
    epd_gfx_text_align_t align;
    int16_t              line_spacing;
    bool                 wrap;
} epd_gfx_text_box_style_t;

/**
 * @brief Measure a single codepoint.
 *
 * @param font Font handle.
 * @param codepoint Unicode codepoint to measure.
 * @param origin Logical text origin.
 * @param style Text style.
 * @param out_rect Pointer to receive layout bounds.
 * @return `EPD_OK` on success, otherwise an error code.
 */
epd_err_t epd_gfx_text_measure_codepoint(const epd_gfx_font_t font,
    uint32_t codepoint, epd_gfx_point_t origin,
    const epd_gfx_text_style_t* style, epd_gfx_rect_t* out_rect);

/**
 * @brief Measure a UTF-8 text string as a single line.
 *
 * @param font Font handle.
 * @param text Null-terminated UTF-8 string.
 * @param origin Logical text origin.
 * @param style Text style.
 * @param out_rect Pointer to receive layout bounds.
 * @return `EPD_OK` on success, otherwise an error code.
 */
epd_err_t epd_gfx_text_measure_utf8(const epd_gfx_font_t font,
    const char* text, epd_gfx_point_t origin,
    const epd_gfx_text_style_t* style, epd_gfx_rect_t* out_rect);

/**
 * @brief Draw a single codepoint.
 *
 * @param canvas Canvas handle.
 * @param font Font handle.
 * @param codepoint Unicode codepoint to draw.
 * @param origin Logical text origin.
 * @param style Text style.
 * @return `EPD_OK` on success, otherwise an error code.
 */
epd_err_t epd_gfx_canvas_draw_codepoint(epd_gfx_canvas_t canvas,
    const epd_gfx_font_t font, uint32_t codepoint, epd_gfx_point_t origin,
    const epd_gfx_text_style_t* style);

/**
 * @brief Draw a UTF-8 text string as a single line.
 *
 * @param canvas Canvas handle.
 * @param font Font handle.
 * @param text Null-terminated UTF-8 string.
 * @param origin Logical text origin.
 * @param style Text style.
 * @return `EPD_OK` on success, otherwise an error code.
 */
epd_err_t epd_gfx_canvas_draw_utf8(epd_gfx_canvas_t canvas,
    const epd_gfx_font_t font, const char* text, epd_gfx_point_t origin,
    const epd_gfx_text_style_t* style);

/**
 * @brief Measure a UTF-8 text string inside a horizontal text box.
 *
 * @param font Font handle.
 * @param text Null-terminated UTF-8 string.
 * @param box Text box bounds.
 * @param style Text box style.
 * @param out_rect Pointer to receive visible layout bounds inside `box`.
 * @return `EPD_OK` on success, otherwise an error code.
 */
epd_err_t epd_gfx_text_measure_utf8_box(const epd_gfx_font_t font,
    const char* text, epd_gfx_rect_t box,
    const epd_gfx_text_box_style_t* style, epd_gfx_rect_t* out_rect);

/**
 * @brief Draw a UTF-8 text string inside a horizontal text box.
 *
 * @param canvas Canvas handle.
 * @param font Font handle.
 * @param text Null-terminated UTF-8 string.
 * @param box Text box bounds.
 * @param style Text box style.
 * @return `EPD_OK` on success, otherwise an error code.
 */
epd_err_t epd_gfx_canvas_draw_utf8_box(epd_gfx_canvas_t canvas,
    const epd_gfx_font_t font, const char* text, epd_gfx_rect_t box,
    const epd_gfx_text_box_style_t* style);

#ifdef __cplusplus
}
#endif

#endif // !_EPD_GFX_TEXT_H_
