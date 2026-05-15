/**
 * @file text.c
 * @brief API for measuring and rendering text into EPD canvas.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2026-05-15
 * @license MIT
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <epd_core/math.h>
#include <epd_gfx/text.h>

typedef struct {
    epd_gfx_canvas_t canvas;
    bool             draw;
} epd_gfx_text_walk_ctx_t;

typedef struct {
    epd_gfx_font_size_info_t size_info;
    epd_gfx_point_t          pen;
    epd_gfx_rect_t           bounds;
    bool                     has_bounds;
} epd_gfx_text_walk_state_t;

static uint16_t epd_gfx_text_metric_int16(int16_t value)
{
    return (value > 0 ? (uint16_t)value : 0U);
}

static uint16_t epd_gfx_text_advance_pen(uint16_t value, uint16_t advance, int16_t spacing)
{
    if (spacing < 0) {
        uint16_t delta = (uint16_t)(0U - (uint16_t)spacing);
        return epd_sat_add_uint16(value, epd_sat_sub_uint16(advance, delta));
    }

    return epd_sat_add_uint16(value, epd_sat_add_uint16(advance, (uint16_t)spacing));
}

static bool epd_gfx_text_check_style(const epd_gfx_text_style_t* style)
{
    return style && style->size != 0U
        && (style->flow == EPD_GFX_TEXT_FLOW_HORIZONTAL
            || style->flow == EPD_GFX_TEXT_FLOW_VERTICAL);
}

static uint32_t epd_gfx_text_normalize_codepoint(uint32_t codepoint)
{
    return (codepoint == '\n' || codepoint == '\t') ? ' ' : codepoint;
}

static void epd_gfx_text_union_rect(epd_gfx_rect_t* rect, const epd_gfx_rect_t* other)
{
    uint16_t x0 = EPD_MIN(rect->x, other->x);
    uint16_t y0 = EPD_MIN(rect->y, other->y);
    uint16_t x1 = EPD_MAX(epd_sat_add_uint16(rect->x, rect->width),
        epd_sat_add_uint16(other->x, other->width));
    uint16_t y1 = EPD_MAX(epd_sat_add_uint16(rect->y, rect->height),
        epd_sat_add_uint16(other->y, other->height));

    rect->x      = x0;
    rect->y      = y0;
    rect->width  = x1 - x0;
    rect->height = y1 - y0;
}

static void epd_gfx_text_add_bounds(epd_gfx_text_walk_state_t* state,
    const epd_gfx_rect_t* cell)
{
    if (!state->has_bounds) {
        state->bounds     = *cell;
        state->has_bounds = true;
        return;
    }

    epd_gfx_text_union_rect(&state->bounds, cell);
}

static epd_err_t epd_gfx_text_decode_utf8(const char** text, uint32_t* out_codepoint)
{
    const uint8_t* src = (const uint8_t*)*text;
    uint32_t       cp  = 0U;
    uint8_t        len = 0U;

    if (src[0] < 0x80U) {
        cp  = src[0];
        len = 1U;
    } else if ((src[0] & 0xE0U) == 0xC0U) {
        cp  = src[0] & 0x1FU;
        len = 2U;
    } else if ((src[0] & 0xF0U) == 0xE0U) {
        cp  = src[0] & 0x0FU;
        len = 3U;
    } else if ((src[0] & 0xF8U) == 0xF0U) {
        cp  = src[0] & 0x07U;
        len = 4U;
    } else {
        return EPD_ERR_INVALID_RESPONSE;
    }

    for (uint8_t i = 1U; i < len; ++i) {
        if ((src[i] & 0xC0U) != 0x80U) {
            return EPD_ERR_INVALID_RESPONSE;
        }
        cp = (cp << 6U) | (src[i] & 0x3FU);
    }

    if ((len == 2U && cp < 0x80U)
        || (len == 3U && cp < 0x800U)
        || (len == 4U && cp < 0x10000U)
        || (cp >= 0xD800U && cp <= 0xDFFFU)
        || cp > 0x10FFFFU) {
        return EPD_ERR_INVALID_RESPONSE;
    }

    *text          = (const char*)(src + len);
    *out_codepoint = cp;
    return EPD_OK;
}

static epd_err_t epd_gfx_text_walk_codepoint(const epd_gfx_font_t font,
    uint32_t codepoint, const epd_gfx_text_style_t* style,
    epd_gfx_text_walk_ctx_t* ctx, epd_gfx_text_walk_state_t* state)
{
    epd_gfx_glyph_key_t key = {
        .codepoint = epd_gfx_text_normalize_codepoint(codepoint),
        .size      = style->size,
    };
    epd_gfx_glyph_t glyph = NULL;
    epd_err_t       ret   = epd_gfx_font_get_glyph(font, key, &glyph);
    if (ret != EPD_OK) {
        return ret;
    }

    uint16_t advance     = epd_gfx_text_metric_int16(epd_gfx_glyph_get_advance(glyph));
    uint16_t line_height = epd_gfx_text_metric_int16(state->size_info.line_height);
    uint16_t ascent      = epd_gfx_text_metric_int16(state->size_info.ascent);
    if (advance == 0U) {
        advance = epd_gfx_glyph_get_width(glyph);
    }
    if (line_height == 0U) {
        line_height = epd_gfx_glyph_get_height(glyph);
    }

    epd_gfx_point_t draw_point = state->pen;
    epd_gfx_rect_t  cell       = { 0 };
    if (style->flow == EPD_GFX_TEXT_FLOW_HORIZONTAL) {
        cell.x       = state->pen.x;
        cell.y       = state->pen.y;
        cell.width   = advance;
        cell.height  = line_height;
        draw_point.y = epd_sat_add_uint16(state->pen.y, ascent);
        state->pen.x = epd_gfx_text_advance_pen(state->pen.x, advance, style->letter_spacing);
    } else if (style->flow == EPD_GFX_TEXT_FLOW_VERTICAL) {
        uint16_t xoffset = (line_height > advance ? (uint16_t)((line_height - advance) / 2U) : 0U);
        draw_point.x = epd_sat_add_uint16(state->pen.x, xoffset);
        draw_point.y = epd_sat_add_uint16(state->pen.y, ascent);
        cell.x       = state->pen.x;
        cell.y       = state->pen.y;
        cell.width   = line_height;
        cell.height  = line_height;
        state->pen.y = epd_gfx_text_advance_pen(state->pen.y, line_height, style->letter_spacing);
    } else {
        (void)epd_gfx_glyph_destroy(glyph);
        return EPD_ERR_INVALID_ARG;
    }

    epd_gfx_text_add_bounds(state, &cell);
    if (ctx->draw) {
        if (style->background != EPD_GFX_BG_TRANSPARENT) {
            ret = epd_gfx_canvas_fill_rect(ctx->canvas, cell, (epd_gfx_color_t)style->background);
            if (ret != EPD_OK) {
                goto clean;
            }
        }
        ret = epd_gfx_canvas_draw_glyph(ctx->canvas, glyph, draw_point, style->color);
        if (ret != EPD_OK) {
            goto clean;
        }
    }

clean:
    (void)epd_gfx_glyph_destroy(glyph);
    return ret;
}

static epd_err_t epd_gfx_text_walk_finish(epd_gfx_text_walk_state_t* state,
    epd_gfx_point_t origin, epd_gfx_rect_t* out_rect)
{
    if (out_rect) {
        *out_rect = state->has_bounds ? state->bounds : (epd_gfx_rect_t){ origin.x, origin.y, 0U, 0U };
    }

    return EPD_OK;
}

static epd_err_t epd_gfx_text_walk_one(const epd_gfx_font_t font,
    uint32_t codepoint, epd_gfx_point_t origin, const epd_gfx_text_style_t* style,
    epd_gfx_text_walk_ctx_t* ctx, epd_gfx_rect_t* out_rect)
{
    if (!font || !epd_gfx_text_check_style(style)) {
        return EPD_ERR_INVALID_ARG;
    }

    epd_gfx_text_walk_state_t state = {
        .pen        = origin,
        .bounds     = { origin.x, origin.y, 0U, 0U },
        .has_bounds = false,
    };
    epd_err_t ret = epd_gfx_font_get_size_info(font, style->size, &state.size_info);
    if (ret != EPD_OK) {
        return ret;
    }
    ret = epd_gfx_text_walk_codepoint(font, codepoint, style, ctx, &state);
    if (ret != EPD_OK) {
        return ret;
    }

    return epd_gfx_text_walk_finish(&state, origin, out_rect);
}

static epd_err_t epd_gfx_text_walk_utf8(const epd_gfx_font_t font,
    const char* text, epd_gfx_point_t origin, const epd_gfx_text_style_t* style,
    epd_gfx_text_walk_ctx_t* ctx, epd_gfx_rect_t* out_rect)
{
    if (!font || !text || !epd_gfx_text_check_style(style)) {
        return EPD_ERR_INVALID_ARG;
    }

    epd_gfx_text_walk_state_t state = {
        .pen        = origin,
        .bounds     = { origin.x, origin.y, 0U, 0U },
        .has_bounds = false,
    };
    epd_err_t ret = epd_gfx_font_get_size_info(font, style->size, &state.size_info);
    if (ret != EPD_OK) {
        return ret;
    }
    while (*text) {
        uint32_t codepoint = 0U;
        ret = epd_gfx_text_decode_utf8(&text, &codepoint);
        if (ret != EPD_OK) {
            return ret;
        }
        ret = epd_gfx_text_walk_codepoint(font, codepoint, style, ctx, &state);
        if (ret != EPD_OK) {
            return ret;
        }
    }

    return epd_gfx_text_walk_finish(&state, origin, out_rect);
}

epd_err_t epd_gfx_text_measure_codepoint(const epd_gfx_font_t font,
    uint32_t codepoint, epd_gfx_point_t origin,
    const epd_gfx_text_style_t* style, epd_gfx_rect_t* out_rect)
{
    if (!out_rect) {
        return EPD_ERR_INVALID_ARG;
    }

    epd_gfx_text_walk_ctx_t ctx = {
        .canvas = NULL,
        .draw   = false,
    };
    return epd_gfx_text_walk_one(font, codepoint, origin, style, &ctx, out_rect);
}

epd_err_t epd_gfx_text_measure_utf8(const epd_gfx_font_t font,
    const char* text, epd_gfx_point_t origin,
    const epd_gfx_text_style_t* style, epd_gfx_rect_t* out_rect)
{
    if (!out_rect) {
        return EPD_ERR_INVALID_ARG;
    }

    epd_gfx_text_walk_ctx_t ctx = {
        .canvas = NULL,
        .draw   = false,
    };
    return epd_gfx_text_walk_utf8(font, text, origin, style, &ctx, out_rect);
}

epd_err_t epd_gfx_canvas_draw_codepoint(epd_gfx_canvas_t canvas,
    const epd_gfx_font_t font, uint32_t codepoint, epd_gfx_point_t origin,
    const epd_gfx_text_style_t* style, epd_gfx_rect_t* out_rect)
{
    if (!canvas) {
        return EPD_ERR_INVALID_ARG;
    }

    epd_gfx_text_walk_ctx_t ctx = {
        .canvas = canvas,
        .draw   = true,
    };
    return epd_gfx_text_walk_one(font, codepoint, origin, style, &ctx, out_rect);
}

epd_err_t epd_gfx_canvas_draw_utf8(epd_gfx_canvas_t canvas,
    const epd_gfx_font_t font, const char* text, epd_gfx_point_t origin,
    const epd_gfx_text_style_t* style, epd_gfx_rect_t* out_rect)
{
    if (!canvas) {
        return EPD_ERR_INVALID_ARG;
    }

    epd_gfx_text_walk_ctx_t ctx = {
        .canvas = canvas,
        .draw   = true,
    };
    return epd_gfx_text_walk_utf8(font, text, origin, style, &ctx, out_rect);
}
