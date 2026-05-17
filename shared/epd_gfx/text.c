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

typedef struct {
    const char* start;
    const char* end;
    const char* next;
    uint16_t    width;
    bool        at_end;
} epd_gfx_text_line_t;

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
    return style && style->size != 0U;
}

static bool epd_gfx_text_check_box_style(const epd_gfx_text_box_style_t* style)
{
    return style && epd_gfx_text_check_style(&style->text)
        && style->align >= 0 && style->align < 3;
}

static uint32_t epd_gfx_text_normalize_codepoint(uint32_t codepoint)
{
    return (codepoint == '\n' || codepoint == '\r' || codepoint == '\t') ? ' ' : codepoint;
}

static uint32_t epd_gfx_text_normalize_box_codepoint(uint32_t codepoint)
{
    return (codepoint == '\r' || codepoint == '\t') ? ' ' : codepoint;
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

static bool epd_gfx_text_intersect_rect(const epd_gfx_rect_t* a,
    const epd_gfx_rect_t* b, epd_gfx_rect_t* out_rect)
{
    uint32_t ax1 = (uint32_t)a->x + a->width;
    uint32_t ay1 = (uint32_t)a->y + a->height;
    uint32_t bx1 = (uint32_t)b->x + b->width;
    uint32_t by1 = (uint32_t)b->y + b->height;
    uint32_t x0  = EPD_MAX(a->x, b->x);
    uint32_t y0  = EPD_MAX(a->y, b->y);
    uint32_t x1  = EPD_MIN(ax1, bx1);
    uint32_t y1  = EPD_MIN(ay1, by1);

    if (x0 >= x1 || y0 >= y1) {
        return false;
    }

    out_rect->x      = (uint16_t)x0;
    out_rect->y      = (uint16_t)y0;
    out_rect->width  = (uint16_t)(x1 - x0);
    out_rect->height = (uint16_t)(y1 - y0);
    return true;
}

static uint16_t epd_gfx_text_glyph_advance(const epd_gfx_glyph_t glyph)
{
    uint16_t advance = epd_gfx_text_metric_int16(epd_gfx_glyph_get_advance(glyph));
    return (advance ? advance : epd_gfx_glyph_get_width(glyph));
}

static uint16_t epd_gfx_text_line_height(const epd_gfx_font_size_info_t* size_info,
    uint16_t fallback)
{
    uint16_t line_height = epd_gfx_text_metric_int16(size_info->line_height);
    return (line_height ? line_height : fallback);
}

static uint16_t epd_gfx_text_line_origin_x(const epd_gfx_rect_t* box,
    uint16_t line_width, epd_gfx_text_align_t align)
{
    if (line_width >= box->width) {
        return box->x;
    }

    uint16_t remaining = box->width - line_width;
    if (align == EPD_GFX_TEXT_ALIGN_CENTER) {
        return epd_sat_add_uint16(box->x, remaining / 2U);
    }
    if (align == EPD_GFX_TEXT_ALIGN_END) {
        return epd_sat_add_uint16(box->x, remaining);
    }

    return box->x;
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
        if (src[i] == 0U || (src[i] & 0xC0U) != 0x80U) {
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
    epd_err_t       ret   = epd_gfx_font_load_glyph(font, key, &glyph);
    if (ret != EPD_OK) {
        return ret;
    }

    uint16_t advance     = epd_gfx_text_glyph_advance(glyph);
    uint16_t line_height = epd_gfx_text_line_height(&state->size_info,
        epd_gfx_glyph_get_height(glyph));
    uint16_t ascent      = epd_gfx_text_metric_int16(state->size_info.ascent);

    epd_gfx_point_t draw_point = state->pen;
    epd_gfx_rect_t  cell       = {
        .x      = state->pen.x,
        .y      = state->pen.y,
        .width  = advance,
        .height = line_height,
    };
    draw_point.y = epd_sat_add_uint16(state->pen.y, ascent);
    state->pen.x = epd_gfx_text_advance_pen(state->pen.x, advance, style->letter_spacing);

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

static epd_err_t epd_gfx_text_scan_box_line(const epd_gfx_font_t font,
    const char* text, uint16_t box_width,
    const epd_gfx_text_box_style_t* style, epd_gfx_text_line_t* out_line)
{
    const char* cursor     = text;
    uint16_t    pen_x      = 0U;
    uint16_t    line_width = 0U;
    bool        has_glyphs = false;

    while (*cursor) {
        const char* codepoint_start = cursor;
        uint32_t    codepoint       = 0U;
        epd_err_t   ret             = epd_gfx_text_decode_utf8(&cursor, &codepoint);
        if (ret != EPD_OK) {
            return ret;
        }
        if (codepoint == '\n') {
            *out_line = (epd_gfx_text_line_t){
                .start  = text,
                .end    = codepoint_start,
                .next   = cursor,
                .width  = line_width,
                .at_end = false,
            };
            return EPD_OK;
        }

        epd_gfx_glyph_key_t key = {
            .codepoint = epd_gfx_text_normalize_box_codepoint(codepoint),
            .size      = style->text.size,
        };
        epd_gfx_glyph_t glyph = NULL;
        ret = epd_gfx_font_load_glyph(font, key, &glyph);
        if (ret != EPD_OK) {
            return ret;
        }

        uint16_t advance    = epd_gfx_text_glyph_advance(glyph);
        uint16_t cell_right = epd_sat_add_uint16(pen_x, advance);
        uint16_t next_width = EPD_MAX(line_width, cell_right);
        (void)epd_gfx_glyph_destroy(glyph);

        if (style->wrap && has_glyphs && next_width > box_width) {
            *out_line = (epd_gfx_text_line_t){
                .start  = text,
                .end    = codepoint_start,
                .next   = codepoint_start,
                .width  = line_width,
                .at_end = false,
            };
            return EPD_OK;
        }

        line_width = next_width;
        has_glyphs = true;
        pen_x      = epd_gfx_text_advance_pen(pen_x, advance, style->text.letter_spacing);
    }

    *out_line = (epd_gfx_text_line_t){
        .start      = text,
        .end        = cursor,
        .next       = cursor,
        .width      = line_width,
        .at_end     = true,
    };
    return EPD_OK;
}

static epd_err_t epd_gfx_text_draw_glyph_clipped(epd_gfx_canvas_t canvas,
    epd_gfx_glyph_t glyph, epd_gfx_point_t point, epd_gfx_rect_t box,
    epd_gfx_color_t color)
{
    const uint8_t* data   = epd_gfx_glyph_get_data(glyph);
    uint16_t       width  = epd_gfx_glyph_get_width(glyph);
    uint16_t       height = epd_gfx_glyph_get_height(glyph);
    if (!data || !width || !height) {
        return EPD_OK;
    }

    epd_err_t ret     = EPD_OK;
    int32_t   glyph_x = (int32_t)point.x + epd_gfx_glyph_get_xoffset(glyph);
    int32_t   glyph_y = (int32_t)point.y + epd_gfx_glyph_get_yoffset(glyph);
    uint32_t  stride  = epd_gfx_glyph_stride(width);
    uint32_t  box_x1  = (uint32_t)box.x + box.width;
    uint32_t  box_y1  = (uint32_t)box.y + box.height;

    for (uint16_t y = 0U; y < height; ++y) {
        int32_t dst_y = glyph_y + y;
        if (dst_y < box.y || (uint32_t)dst_y >= box_y1) {
            continue;
        }

        const uint8_t* row = data + (uint32_t)y * stride;
        for (uint16_t x = 0U; x < width; ++x) {
            int32_t dst_x = glyph_x + x;
            if (dst_x < box.x || (uint32_t)dst_x >= box_x1) {
                continue;
            }
            if (row[x / 8U] & (uint8_t)(0x80U >> (x & 7U))) {
                EPD_CHECK_RET(epd_gfx_canvas_draw_pixel(canvas, (epd_gfx_point_t){
                    (uint16_t)dst_x,
                    (uint16_t)dst_y,
                }, color));
            }
        }
    }

    return EPD_OK;
}

static epd_err_t epd_gfx_text_walk_box_line(const epd_gfx_font_t font,
    epd_gfx_canvas_t canvas, const epd_gfx_text_line_t* line, epd_gfx_rect_t box,
    uint16_t line_y, const epd_gfx_font_size_info_t* size_info,
    const epd_gfx_text_box_style_t* style, epd_gfx_text_walk_state_t* state)
{
    const char*    cursor      = line->start;
    epd_gfx_rect_t draw_box    = box;
    uint16_t       pen_x       = epd_gfx_text_line_origin_x(&box, line->width, style->align);
    uint16_t       line_height = epd_gfx_text_line_height(size_info, style->text.size);
    uint16_t       ascent      = epd_gfx_text_metric_int16(size_info->ascent);
    uint32_t       box_y1      = (uint32_t)box.y + box.height;

    if (canvas) {
        epd_gfx_rect_t canvas_rect = {
            .x      = 1U,
            .y      = 1U,
            .width  = epd_gfx_canvas_get_logical_width(canvas),
            .height = epd_gfx_canvas_get_logical_height(canvas),
        };
        if (!epd_gfx_text_intersect_rect(&draw_box, &canvas_rect, &draw_box)) {
            canvas = NULL;
        }
    }

    while (cursor != line->end) {
        uint32_t  codepoint = 0U;
        epd_err_t ret       = epd_gfx_text_decode_utf8(&cursor, &codepoint);
        if (ret != EPD_OK) {
            return ret;
        }

        epd_gfx_glyph_key_t key = {
            .codepoint = epd_gfx_text_normalize_box_codepoint(codepoint),
            .size      = style->text.size,
        };
        epd_gfx_glyph_t glyph = NULL;
        ret = epd_gfx_font_load_glyph(font, key, &glyph);
        if (ret != EPD_OK) {
            return ret;
        }

        uint16_t advance = epd_gfx_text_glyph_advance(glyph);
        epd_gfx_rect_t cell = {
            .x      = pen_x,
            .y      = line_y,
            .width  = advance,
            .height = line_height,
        };
        epd_gfx_rect_t visible;
        if (epd_gfx_text_intersect_rect(&cell, &box, &visible)) {
            epd_gfx_text_add_bounds(state, &visible);
            if (canvas && line_y < box_y1) {
                epd_gfx_rect_t fill;
                if (style->text.background != EPD_GFX_BG_TRANSPARENT) {
                    if (!epd_gfx_text_intersect_rect(&visible, &draw_box, &fill)) {
                        goto skip_draw;
                    }
                    ret = epd_gfx_canvas_fill_rect(canvas, fill,
                        (epd_gfx_color_t)style->text.background);
                    if (ret != EPD_OK) {
                        goto clean;
                    }
                }
                ret = epd_gfx_text_draw_glyph_clipped(canvas, glyph, (epd_gfx_point_t){
                    pen_x,
                    epd_sat_add_uint16(line_y, ascent),
                }, draw_box, style->text.color);
                if (ret != EPD_OK) {
                    goto clean;
                }
            }
        }

skip_draw:
        pen_x = epd_gfx_text_advance_pen(pen_x, advance, style->text.letter_spacing);

clean:
        (void)epd_gfx_glyph_destroy(glyph);
        if (ret != EPD_OK) {
            return ret;
        }
    }

    return EPD_OK;
}

static epd_err_t epd_gfx_text_walk_utf8_box(const epd_gfx_font_t font,
    epd_gfx_canvas_t canvas, const char* text, epd_gfx_rect_t box,
    const epd_gfx_text_box_style_t* style, epd_gfx_rect_t* out_rect)
{
    if (!font || !text || !epd_gfx_text_check_box_style(style)) {
        return EPD_ERR_INVALID_ARG;
    }

    epd_gfx_text_walk_state_t state = {
        .bounds     = { box.x, box.y, 0U, 0U },
        .has_bounds = false,
    };
    epd_err_t ret = epd_gfx_font_get_size_info(font, style->text.size, &state.size_info);
    if (ret != EPD_OK) {
        return ret;
    }

    const char* cursor      = text;
    uint16_t    line_y      = box.y;
    uint16_t    line_height = epd_gfx_text_line_height(&state.size_info, style->text.size);
    while (true) {
        epd_gfx_text_line_t line;
        ret = epd_gfx_text_scan_box_line(font, cursor, box.width, style, &line);
        if (ret != EPD_OK) {
            return ret;
        }
        ret = epd_gfx_text_walk_box_line(font, canvas, &line, box, line_y,
            &state.size_info, style, &state);
        if (ret != EPD_OK) {
            return ret;
        }
        if (line.at_end) {
            break;
        }

        cursor = line.next;
        line_y = epd_gfx_text_advance_pen(line_y, line_height, style->line_spacing);
    }

    if (out_rect) {
        *out_rect = state.has_bounds ? state.bounds : (epd_gfx_rect_t){ box.x, box.y, 0U, 0U };
    }

    return EPD_OK;
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
    const epd_gfx_text_style_t* style)
{
    if (!canvas) {
        return EPD_ERR_INVALID_ARG;
    }

    epd_gfx_text_walk_ctx_t ctx = {
        .canvas = canvas,
        .draw   = true,
    };
    return epd_gfx_text_walk_one(font, codepoint, origin, style, &ctx, NULL);
}

epd_err_t epd_gfx_canvas_draw_utf8(epd_gfx_canvas_t canvas,
    const epd_gfx_font_t font, const char* text, epd_gfx_point_t origin,
    const epd_gfx_text_style_t* style)
{
    if (!canvas) {
        return EPD_ERR_INVALID_ARG;
    }

    epd_gfx_text_walk_ctx_t ctx = {
        .canvas = canvas,
        .draw   = true,
    };
    return epd_gfx_text_walk_utf8(font, text, origin, style, &ctx, NULL);
}

epd_err_t epd_gfx_text_measure_utf8_box(const epd_gfx_font_t font,
    const char* text, epd_gfx_rect_t box,
    const epd_gfx_text_box_style_t* style, epd_gfx_rect_t* out_rect)
{
    if (!out_rect) {
        return EPD_ERR_INVALID_ARG;
    }

    return epd_gfx_text_walk_utf8_box(font, NULL, text, box, style, out_rect);
}

epd_err_t epd_gfx_canvas_draw_utf8_box(epd_gfx_canvas_t canvas,
    const epd_gfx_font_t font, const char* text, epd_gfx_rect_t box,
    const epd_gfx_text_box_style_t* style)
{
    if (!canvas) {
        return EPD_ERR_INVALID_ARG;
    }

    return epd_gfx_text_walk_utf8_box(font, canvas, text, box, style, NULL);
}
