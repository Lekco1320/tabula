/**
 * @file canvas.c
 * @brief Canvas API for drawing into EPD buffers (native or planes).
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2025-11-21
 * @license MIT
 */

#include <string.h>
#include <stdlib.h>
#include <epd_core/math.h>
#include <epd_core/common.h>

#include "epd_gfx/codec.h"
#include "epd_gfx/canvas.h"
#include "epd_gfx/canvas_impl.h"
#include "epd_gfx/glyph.h"
#include "epd_gfx/glyph_impl.h"

static EPD_INLINE void swap(uint16_t* a, uint16_t* b)
{
    uint16_t temp = *a;
    *a = *b;
    *b = temp;
}

static EPD_INLINE bool epd_gfx_canvas_in_planes(const epd_gfx_canvas_t canvas)
{
    return (canvas->flags & 1);
}

static void epd_gfx_canvas_map_rot0(const epd_gfx_canvas_t canvas, uint16_t* px, uint16_t* py)
{
    --*px;
    --*py;
}

static void epd_gfx_canvas_map_rot90(const epd_gfx_canvas_t canvas, uint16_t* px, uint16_t* py)
{
    --*px;
    --*py;
    uint16_t x = *px;
    *px = *py;
    *py = canvas->height - 1U - x;
}

static void epd_gfx_canvas_map_rot180(const epd_gfx_canvas_t canvas, uint16_t* px, uint16_t* py)
{
    --*px;
    --*py;
    *px = canvas->width  - 1U - *px;
    *py = canvas->height - 1U - *py;
}

static void epd_gfx_canvas_map_rot270(const epd_gfx_canvas_t canvas, uint16_t* px, uint16_t* py)
{
    --*px;
    --*py;
    uint16_t x = *px;
    *px = canvas->width - 1U - *py;
    *py = x;
}

static EPD_INLINE bool epd_gfx_check_bound(const epd_gfx_canvas_t canvas, uint16_t x, uint16_t y)
{
    return x <= canvas->width && y <= canvas->height && x > 0 && y > 0;
}

static EPD_INLINE bool epd_gfx_check_bound_mapped(const epd_gfx_canvas_t canvas, uint16_t x, uint16_t y)
{
    return x < canvas->width && y < canvas->height;
}

epd_err_t epd_gfx_canvas_create(const epd_gfx_canvas_config_t* config, epd_gfx_canvas_t* out_canvas)
{
    if (!config || !out_canvas) {
        return EPD_ERR_INVALID_ARG;
    }

    epd_err_t        ret    = EPD_OK;
    epd_gfx_canvas_t canvas = calloc(1, sizeof(struct epd_gfx_canvas_impl));
    if (!canvas) {
        return EPD_ERR_NO_MEM;
    }

    canvas->width  = config->width;
    canvas->height = config->height;
    switch (config->format)
    {
    case EPD_GFX_FORMAT_NATIVE:
        canvas->flags      = 0;
        canvas->buf_stride = epd_gfx_native_stride(canvas->width);
        canvas->buf_size   = (uint32_t)canvas->buf_stride * canvas->height;
        canvas->buf_native = (uint8_t*)calloc(canvas->buf_size, sizeof(uint8_t));
        if (!canvas->buf_native) {
            ret = EPD_ERR_NO_MEM;
            goto fail;
        }
        break;

    case EPD_GFX_FORMAT_PLANES:
        canvas->flags      = 1;
        canvas->buf_stride = epd_gfx_planes_stride(canvas->width);
        canvas->buf_size   = (uint32_t)canvas->buf_stride * canvas->height;
        canvas->buf_wht    = (uint8_t*)calloc(canvas->buf_size, sizeof(uint8_t));
        canvas->buf_red    = (uint8_t*)calloc(canvas->buf_size, sizeof(uint8_t));
        if (!canvas->buf_wht || !canvas->buf_red) {
            ret = EPD_ERR_NO_MEM;
            goto fail;
        }
        break;

    default:
        ret = EPD_ERR_INVALID_ARG;
        goto fail;
    }

    EPD_CHECK_GOTO(epd_gfx_canvas_set_rotation(canvas, config->rotation), fail);
    
    *out_canvas = canvas;
    return EPD_OK;

fail:
    if (canvas) {
        (void)epd_gfx_canvas_destroy(canvas);
    }
    return ret;
}

epd_err_t epd_gfx_canvas_destroy(epd_gfx_canvas_t canvas)
{
    if (!canvas) {
        return EPD_OK;
    }

    if (canvas->buf_native) {
        free(canvas->buf_native);
        canvas->buf_native = NULL;
    }
    if (canvas->buf_wht) {
        free(canvas->buf_wht);
        canvas->buf_wht = NULL;
    }
    if (canvas->buf_red) {
        free(canvas->buf_red);
        canvas->buf_red = NULL;
    }
    free(canvas);

    return EPD_OK;
}

epd_gfx_rotation_t epd_gfx_canvas_get_rotation(const epd_gfx_canvas_t canvas)
{
    return (!canvas ? EPD_GFX_ROTATE_UNKNOWN : (epd_gfx_rotation_t)((canvas->flags >> 1) & 3));
}

epd_gfx_format_t epd_gfx_canvas_get_format(const epd_gfx_canvas_t canvas)
{
    return (!canvas ? EPD_GFX_FORMAT_UNKNOWN : (epd_gfx_format_t)(canvas->flags & 1));
}

epd_err_t epd_gfx_canvas_set_rotation(epd_gfx_canvas_t canvas, epd_gfx_rotation_t rotation)
{
    if (!canvas) {
        return EPD_ERR_INVALID_ARG;
    }

    switch (rotation)
    {
    case EPD_GFX_ROTATE_0:
        canvas->flags      = (canvas->flags & 0xF9) | (EPD_GFX_ROTATE_0 << 1);
        canvas->map_fn     = epd_gfx_canvas_map_rot0;
        canvas->lwidth_fn  = epd_gfx_canvas_get_width;
        canvas->lheight_fn = epd_gfx_canvas_get_height;
        break;
    
    case EPD_GFX_ROTATE_90:
        canvas->flags      = (canvas->flags & 0xF9) | (EPD_GFX_ROTATE_90 << 1);
        canvas->map_fn     = epd_gfx_canvas_map_rot90;
        canvas->lwidth_fn  = epd_gfx_canvas_get_height;
        canvas->lheight_fn = epd_gfx_canvas_get_width;
        break;

    case EPD_GFX_ROTATE_180:
        canvas->flags      = (canvas->flags & 0xF9) | (EPD_GFX_ROTATE_180 << 1);
        canvas->map_fn     = epd_gfx_canvas_map_rot180;
        canvas->lwidth_fn  = epd_gfx_canvas_get_width;
        canvas->lheight_fn = epd_gfx_canvas_get_height;
        break;

    case EPD_GFX_ROTATE_270:
        canvas->flags      = (canvas->flags & 0xF9) | (EPD_GFX_ROTATE_270 << 1);
        canvas->map_fn     = epd_gfx_canvas_map_rot270;
        canvas->lwidth_fn  = epd_gfx_canvas_get_height;
        canvas->lheight_fn = epd_gfx_canvas_get_width;
        break;

    default:
        return EPD_ERR_INVALID_ARG;
    }

    return EPD_OK;
}

uint16_t epd_gfx_canvas_get_width(const epd_gfx_canvas_t canvas)
{
    return (!canvas ? 0 : canvas->width);
}

uint16_t epd_gfx_canvas_get_height(const epd_gfx_canvas_t canvas)
{
    return (!canvas ? 0 : canvas->height);
}

uint16_t epd_gfx_canvas_get_logical_width(const epd_gfx_canvas_t canvas)
{
    return (!canvas ? 0 : canvas->lwidth_fn(canvas));
}

uint16_t epd_gfx_canvas_get_logical_height(const epd_gfx_canvas_t canvas)
{
    return (!canvas ? 0 : canvas->lheight_fn(canvas));
}

epd_gfx_color_t epd_gfx_canvas_get_pixel(const epd_gfx_canvas_t canvas,
    epd_gfx_point_t point)
{
    if (!canvas) {
        return EPD_GFX_WHITE;
    }

    canvas->map_fn(canvas, &point.x, &point.y);
    if (!epd_gfx_check_bound_mapped(canvas, point.x, point.y)) {
        return EPD_GFX_WHITE;
    }

    if (epd_gfx_canvas_in_planes(canvas)) {
        uint32_t index = (uint32_t)canvas->buf_stride * point.y + point.x / 8U;
        uint8_t  digit = point.x % 8U;
        return epd_gfx_planes_get_pixel(canvas->buf_wht[index], canvas->buf_red[index], digit);
    } else {
        uint32_t index = (uint32_t)canvas->buf_stride * point.y + point.x / 2U;
        uint8_t  digit = point.x % 2U;
        return epd_gfx_native_get_pixel(canvas->buf_native[index], digit);
    }
}

epd_err_t epd_gfx_canvas_clear(epd_gfx_canvas_t canvas)
{
    return epd_gfx_canvas_fill(canvas, EPD_GFX_WHITE);
}

epd_err_t epd_gfx_canvas_fill(epd_gfx_canvas_t canvas, epd_gfx_color_t color)
{
    if (!canvas) {
        return EPD_ERR_INVALID_ARG;
    }

    if (epd_gfx_canvas_in_planes(canvas)) {
        epd_gfx_planes_set_bytes(canvas->buf_wht, canvas->buf_red, canvas->buf_size, color);
    } else {
        epd_gfx_native_set_bytes(canvas->buf_native, canvas->buf_size, color);
    }

    return EPD_OK;
}

epd_err_t epd_gfx_canvas_draw_pixel(epd_gfx_canvas_t canvas,
    epd_gfx_point_t point, epd_gfx_color_t color)
{
    if (!canvas) {
        return EPD_ERR_INVALID_ARG;
    }

    canvas->map_fn(canvas, &point.x, &point.y);
    if (!epd_gfx_check_bound_mapped(canvas, point.x, point.y)) {
        return EPD_ERR_INVALID_ARG;
    }

    if (epd_gfx_canvas_in_planes(canvas)) {
        uint16_t index = canvas->buf_stride * point.y + point.x / 8U;
        uint8_t  digit = point.x % 8;
        epd_gfx_planes_set_pixel(canvas->buf_wht + index, canvas->buf_red + index, digit, color);
    } else {
        uint32_t index = canvas->buf_stride * point.y + point.x / 2U;
        uint8_t  digit = point.x % 2;
        epd_gfx_native_set_pixel(canvas->buf_native + index, digit, color);
    }
    
    return EPD_OK;
}

static epd_err_t epd_gfx_canvas_draw_hline_impl(epd_gfx_canvas_t canvas,
    uint16_t x, uint16_t y, uint16_t w, epd_gfx_color_t color)
{
    uint16_t x1 = epd_sat_add_uint16(x, w - 1);
    x1 = EPD_MIN(x1, canvas->width - 1);
    if (epd_gfx_canvas_in_planes(canvas)) {
        uint16_t b0 = x / 8U;
        uint16_t b1 = x1 / 8U;
        uint8_t  d0 = (uint8_t)x % 8U;
        uint8_t  d1 = (uint8_t)x1 % 8U;
        if (b0 == b1) {
            uint32_t index = canvas->buf_stride * y + b0;
            epd_gfx_planes_set_range_pixels(canvas->buf_wht + index, canvas->buf_red + index, d0, d1, color);
            return EPD_OK;
        }
        if (d0) {
            uint32_t index = canvas->buf_stride * y + b0;
            epd_gfx_planes_set_range_pixels(canvas->buf_wht + index, canvas->buf_red + index, d0, 7, color);
            ++b0;
        }
        if (d1 < 7) {
            uint32_t index = canvas->buf_stride * y + b1;
            epd_gfx_planes_set_range_pixels(canvas->buf_wht + index, canvas->buf_red + index, 0, d1, color);
            --b1;
        }
        if (b1 >= b0) {
            uint16_t len   = b1 - b0 + 1U;
            uint32_t index = canvas->buf_stride * y + b0;
            epd_gfx_planes_set_bytes(canvas->buf_wht + index, canvas->buf_red + index, len, color);
        }
    } else {
        uint16_t b0 = x / 2U;
        uint16_t b1 = x1 / 2U;
        uint8_t  d0 = (uint8_t)x % 2U;
        uint8_t  d1 = (uint8_t)x1 % 2U;
        if (d0) {
            uint32_t index = canvas->buf_stride * y + b0;
            epd_gfx_native_set_pixel(canvas->buf_native + index, 1, color);
            ++b0;
        }
        if (!d1) {
            uint32_t index = canvas->buf_stride * y + b1;
            epd_gfx_native_set_pixel(canvas->buf_native + index, 0, color);
            --b1;
        }
        if (b1 >= b0) {
            uint16_t len   = b1 - b0 + 1U;
            uint32_t index = canvas->buf_stride * y + b0;
            epd_gfx_native_set_bytes(canvas->buf_native + index, len, color);
        }
    }
    return EPD_OK;
}

static epd_err_t epd_gfx_canvas_draw_vline_impl(epd_gfx_canvas_t canvas,
    uint16_t x, uint16_t y, uint16_t h, epd_gfx_color_t color)
{
    uint16_t y1 = epd_sat_add_uint16(y, h - 1);
    y1 = EPD_MIN(y1, canvas->height - 1);
    if (epd_gfx_canvas_in_planes(canvas)) {
        uint8_t  digit = (uint8_t)x % 8U;
        uint32_t index = canvas->buf_stride * y + x / 8U;
        for (uint16_t yy = y; yy <= y1; ++yy) {
            epd_gfx_planes_set_pixel(canvas->buf_wht + index, canvas->buf_red + index, digit, color);
            index += canvas->buf_stride;
        }
    } else {
        uint8_t  digit = (uint8_t)x % 2U;
        uint32_t index = canvas->buf_stride * y + x / 2U;
        for (uint16_t yy = y; yy <= y1; ++yy) {
            epd_gfx_native_set_pixel(canvas->buf_native + index, digit, color);
            index += canvas->buf_stride;
        }
    }
    return EPD_OK;
}

epd_err_t epd_gfx_canvas_draw_hline(epd_gfx_canvas_t canvas,
    epd_gfx_point_t start, uint16_t w, epd_gfx_color_t color)
{
    if (!canvas) {
        return EPD_ERR_INVALID_ARG;
    }
    if (!w) {
        return EPD_OK;
    }
    if (w == 1) {
        return epd_gfx_canvas_draw_pixel(canvas, start, color);
    }

    canvas->map_fn(canvas, &start.x, &start.y);
    if (!epd_gfx_check_bound_mapped(canvas, start.x, start.y)) {
        return EPD_ERR_INVALID_ARG;
    }

    epd_gfx_rotation_t rotation = epd_gfx_canvas_get_rotation(canvas);
    switch (rotation)
    {
    case EPD_GFX_ROTATE_0:
        return epd_gfx_canvas_draw_hline_impl(canvas, start.x, start.y, w, color);    

    case EPD_GFX_ROTATE_90: {
        uint16_t y0 = epd_sat_sub_uint16(start.y, w - 1);
        uint16_t hh = start.y - y0 + 1;
        return epd_gfx_canvas_draw_vline_impl(canvas, start.x, y0, hh, color);
    }

    case EPD_GFX_ROTATE_180: {
        uint16_t x0 = epd_sat_sub_uint16(start.x, w - 1);
        uint16_t ww = start.x - x0 + 1;
        return epd_gfx_canvas_draw_hline_impl(canvas, x0, start.y, ww, color);
    }

    case EPD_GFX_ROTATE_270:
        return epd_gfx_canvas_draw_vline_impl(canvas, start.x, start.y, w, color);

    default:
        return EPD_ERR_INVALID_ARG;
    }
}

epd_err_t epd_gfx_canvas_draw_vline(epd_gfx_canvas_t canvas,
    epd_gfx_point_t start, uint16_t h, epd_gfx_color_t color)
{
    if (!canvas) {
        return EPD_ERR_INVALID_ARG;
    }
    if (!h) {
        return EPD_OK;
    }
    if (h == 1) {
        return epd_gfx_canvas_draw_pixel(canvas, start, color);
    }

    canvas->map_fn(canvas, &start.x, &start.y);
    if (!epd_gfx_check_bound_mapped(canvas, start.x, start.y)) {
        return EPD_ERR_INVALID_ARG;
    }

    epd_gfx_rotation_t rotation = epd_gfx_canvas_get_rotation(canvas);
    switch (rotation)
    {
    case EPD_GFX_ROTATE_0:
        return epd_gfx_canvas_draw_vline_impl(canvas, start.x, start.y, h, color);

    case EPD_GFX_ROTATE_90:
        return epd_gfx_canvas_draw_hline_impl(canvas, start.x, start.y, h, color);

    case EPD_GFX_ROTATE_180: {
        uint16_t y0 = epd_sat_sub_uint16(start.y, h - 1);
        uint16_t hh = start.y - y0 + 1;
        return epd_gfx_canvas_draw_vline_impl(canvas, start.x, y0, hh, color);
    }

    case EPD_GFX_ROTATE_270: {
        uint16_t x0 = epd_sat_sub_uint16(start.x, h - 1);
        uint16_t ww = start.x - x0 + 1;
        return epd_gfx_canvas_draw_hline_impl(canvas, x0, start.y, ww, color);
    }

    default:
        return EPD_ERR_INVALID_ARG;
    }
}

epd_err_t epd_gfx_canvas_draw_rect(epd_gfx_canvas_t canvas,
    epd_gfx_rect_t rect, epd_gfx_color_t color)
{
    if (!canvas) {
        return EPD_ERR_INVALID_ARG;
    }
    if (!rect.width || !rect.height) {
        return EPD_OK;
    }

    epd_err_t ret = EPD_OK;

    // Draw Top Edge
    EPD_CHECK_RET(epd_gfx_canvas_draw_hline(canvas, (epd_gfx_point_t){ rect.x, rect.y },
        rect.width, color));

    // Draw Bottom Edge
    uint16_t lw = epd_gfx_canvas_get_logical_width(canvas);
    uint16_t lh = epd_gfx_canvas_get_logical_height(canvas);
    uint16_t x1 = epd_sat_add_uint16(rect.x, rect.width - 1);
    uint16_t y1 = epd_sat_add_uint16(rect.y, rect.height - 1);
    if (rect.height > 1 && y1 <= lh) {
        EPD_CHECK_RET(epd_gfx_canvas_draw_hline(canvas, (epd_gfx_point_t){ rect.x, y1 },
            rect.width, color));
    }

    // Draw Side Edges (excluding corners)
    if (rect.height > 2) {
        uint16_t y_inner = epd_sat_add_uint16(rect.y, 1);
        uint16_t h_inner = rect.height - 2;
        if (y_inner <= lh) {
            EPD_CHECK_RET(epd_gfx_canvas_draw_vline(canvas, (epd_gfx_point_t){ rect.x, y_inner },
                h_inner, color));
            if (rect.width > 1 && x1 <= lw) {
                EPD_CHECK_RET(epd_gfx_canvas_draw_vline(canvas, (epd_gfx_point_t){ x1, y_inner },
                    h_inner, color));
            }
        }
    }

    return EPD_OK;
}

epd_err_t epd_gfx_canvas_fill_rect(epd_gfx_canvas_t canvas,
    epd_gfx_rect_t rect, epd_gfx_color_t color)
{
    if (!canvas) {
        return EPD_ERR_INVALID_ARG;
    }
    if (!rect.width || !rect.height) {
        return EPD_OK;
    }

    uint16_t px = rect.x;
    uint16_t py = rect.y;
    canvas->map_fn(canvas, &px, &py);
    
    uint16_t x0, y0, x1, y1;
    epd_gfx_rotation_t rotation = epd_gfx_canvas_get_rotation(canvas);
    switch (rotation)
    {
    case EPD_GFX_ROTATE_0:
        x0 = px;
        y0 = py;
        x1 = epd_sat_add_uint16(px, rect.width - 1);
        y1 = epd_sat_add_uint16(py, rect.height - 1);
        break;
    
    case EPD_GFX_ROTATE_90:
        x0 = px;
        y0 = epd_sat_sub_uint16(py, rect.width - 1);
        x1 = epd_sat_add_uint16(px, rect.height - 1);
        y1 = py;
        break;

    case EPD_GFX_ROTATE_180:
        x0 = epd_sat_sub_uint16(px, rect.width - 1);
        y0 = epd_sat_sub_uint16(py, rect.height - 1);
        x1 = px;
        y1 = py;
        break;

    case EPD_GFX_ROTATE_270:
        x0 = epd_sat_sub_uint16(px, rect.height - 1);
        y0 = py;
        x1 = px;
        y1 = epd_sat_add_uint16(py, rect.width - 1);
        break;

    default:
        return EPD_ERR_INVALID_ARG;
    }

    if (x0 >= canvas->width || y0 >= canvas->height) {
        return EPD_OK;
    }

    x1 = EPD_MIN(x1, canvas->width - 1);
    y1 = EPD_MIN(y1, canvas->height - 1);
    if (x0 > x1 || y0 > y1) {
        return EPD_OK;
    }

    uint16_t width = x1 - x0 + 1;
    epd_err_t ret = EPD_OK;
    for (uint16_t yy = y0; yy <= y1; ++yy) {
        EPD_CHECK_RET(epd_gfx_canvas_draw_hline_impl(canvas, x0, yy, width, color));
    }
    return EPD_OK;
}

epd_err_t epd_gfx_canvas_flush(const epd_gfx_canvas_t canvas, const epd_gfx_frame_view_sink_t* sink)
{
    if (!canvas || !sink || !sink->flush_impl) {
        return EPD_ERR_INVALID_ARG;
    }

    epd_gfx_frame_view_t frame_view = {
        .format  = epd_gfx_canvas_get_format(canvas),
        .width   = canvas->width,
        .height  = canvas->height,
        .stride  = canvas->buf_stride,
        .buf_wht = NULL,
        .buf_red = NULL,
    };
    if (frame_view.format) {
        frame_view.buf_wht = canvas->buf_wht;
        frame_view.buf_red = canvas->buf_red;
    } else {
        frame_view.buf_native = canvas->buf_native;
    }

    return sink->flush_impl(sink->context, &frame_view);
}

epd_err_t epd_gfx_canvas_clone(const epd_gfx_canvas_t canvas, epd_gfx_canvas_t* out_canvas)
{
    if (!canvas || !out_canvas) {
        return EPD_ERR_INVALID_ARG;
    }

    epd_gfx_canvas_t new_canvas = calloc(1, sizeof(struct epd_gfx_canvas_impl));
    if (!new_canvas) {
        return EPD_ERR_NO_MEM;
    }

    new_canvas->width      = canvas->width;
    new_canvas->height     = canvas->height;
    new_canvas->flags      = canvas->flags;
    new_canvas->map_fn     = canvas->map_fn;
    new_canvas->lwidth_fn  = canvas->lwidth_fn;
    new_canvas->lheight_fn = canvas->lheight_fn;
    new_canvas->buf_stride = canvas->buf_stride;
    new_canvas->buf_size   = canvas->buf_size;

    epd_err_t ret = EPD_OK;
    if (epd_gfx_canvas_in_planes(canvas)) {
        new_canvas->buf_wht = (uint8_t*)malloc(canvas->buf_size * sizeof(uint8_t));
        new_canvas->buf_red = (uint8_t*)malloc(canvas->buf_size * sizeof(uint8_t));
        if (!new_canvas->buf_wht || !new_canvas->buf_red) {
            ret = EPD_ERR_NO_MEM;
            goto fail;
        }
        memcpy(new_canvas->buf_wht, canvas->buf_wht, canvas->buf_size * sizeof(uint8_t));
        memcpy(new_canvas->buf_red, canvas->buf_red, canvas->buf_size * sizeof(uint8_t));
    } else {
        new_canvas->buf_native = (uint8_t*)malloc(canvas->buf_size * sizeof(uint8_t));
        if (!new_canvas->buf_native) {
            ret = EPD_ERR_NO_MEM;
            goto fail;
        }
        memcpy(new_canvas->buf_native, canvas->buf_native, canvas->buf_size * sizeof(uint8_t));
    }
    
    *out_canvas = new_canvas;
    return EPD_OK;

fail:
    if (new_canvas) {
        (void)epd_gfx_canvas_destroy(new_canvas);
    }
    return ret;
}

epd_err_t epd_gfx_canvas_load_native(epd_gfx_canvas_t canvas, const uint8_t* data,
    uint32_t size)
{
    if (!canvas || !data || epd_gfx_canvas_get_format(canvas) != EPD_GFX_FORMAT_NATIVE) {
        return EPD_ERR_INVALID_ARG;
    }
    
    uint32_t stride   = epd_gfx_native_stride(canvas->width);
    uint32_t expected = stride * canvas->height;
    if (size != expected) {
        return EPD_ERR_INVALID_SIZE;
    }

    memcpy(canvas->buf_native, data, expected);
    return EPD_OK;
}

epd_err_t epd_gfx_canvas_load_planes(epd_gfx_canvas_t canvas, const uint8_t* pwht,
    const uint8_t* pred, uint32_t size)
{
    if (!canvas || !pwht || !pred || epd_gfx_canvas_get_format(canvas) != EPD_GFX_FORMAT_PLANES) {
        return EPD_ERR_INVALID_ARG;
    }

    uint32_t stride   = epd_gfx_planes_stride(canvas->width);
    uint32_t expected = stride * canvas->height;
    if (size != expected) {
        return EPD_ERR_INVALID_SIZE;
    }

    memcpy(canvas->buf_wht, pwht, expected);
    memcpy(canvas->buf_red, pred, expected);
    return EPD_OK;
}

typedef struct {
    uint16_t src_x;
    uint16_t src_y;
    uint16_t dst_x;
    uint16_t dst_y;
    uint16_t width;
    uint16_t height;
} epd_gfx_glyph_clip_t;

static bool epd_gfx_canvas_clip_glyph_axis(uint16_t origin, int16_t offset,
    uint16_t extent, uint16_t limit, uint16_t* out_src, uint16_t* out_dst,
    uint16_t* out_length)
{
    uint16_t src = 0U;
    uint16_t dst = 0U;

    if (!extent || !limit) {
        return false;
    }

    if (offset < 0) {
        uint16_t delta = (uint16_t)(0U - (uint16_t)offset);
        if (delta >= origin) {
            src = delta - origin + 1U;
            if (src >= extent) {
                return false;
            }
        } else {
            dst = origin - delta - 1U;
        }
    } else {
        uint16_t top = epd_sat_add_uint16(origin, (uint16_t)offset);
        if (!top) {
            src = 1U;
            if (src >= extent) {
                return false;
            }
        } else {
            if (top > limit) {
                return false;
            }
            dst = top - 1U;
        }
    }

    uint16_t available = limit - dst;
    uint16_t length    = EPD_MIN((uint16_t)(extent - src), available);
    if (!length) {
        return false;
    }

    *out_src    = src;
    *out_dst    = dst;
    *out_length = length;
    return true;
}

static bool epd_gfx_canvas_clip_glyph(const epd_gfx_glyph_t glyph, uint16_t x, uint16_t y,
    uint16_t width, uint16_t height, epd_gfx_glyph_clip_t* out_clip)
{
    return epd_gfx_canvas_clip_glyph_axis(x, glyph->xoffset, glyph->width, width,
        &out_clip->src_x, &out_clip->dst_x, &out_clip->width)
        && epd_gfx_canvas_clip_glyph_axis(y, glyph->yoffset, glyph->height, height,
        &out_clip->src_y, &out_clip->dst_y, &out_clip->height);
}

static epd_err_t epd_gfx_canvas_draw_glyph_planes_rot0(epd_gfx_canvas_t canvas, epd_gfx_glyph_t glyph,
    const epd_gfx_glyph_clip_t* clip, epd_gfx_color_t color)
{
    uint16_t glyph_stride = (uint16_t)epd_gfx_glyph_stride(glyph->width);
    for (uint16_t y = 0U; y < clip->height; ++y) {
        const uint8_t* src_row = glyph->data + (uint32_t)(clip->src_y + y) * glyph_stride;
        uint8_t*       dst_wht = canvas->buf_wht + (uint32_t)(clip->dst_y + y) * canvas->buf_stride;
        uint8_t*       dst_red = canvas->buf_red + (uint32_t)(clip->dst_y + y) * canvas->buf_stride;

        uint16_t src_x     = clip->src_x;
        uint16_t dst_x     = clip->dst_x;
        uint16_t remaining = clip->width;
        while (remaining) {
            uint8_t dst_bit = (uint8_t)(dst_x & 7U);
            uint8_t count   = (uint8_t)EPD_MIN((uint16_t)(8U - dst_bit), remaining);
            uint8_t mask    = epd_gfx_mono_row_mask(src_row, src_x, dst_bit, count);
            if (mask) {
                uint16_t index = dst_x / 8U;
                epd_gfx_planes_set_pixel_impl(dst_wht + index, dst_red + index, mask, color);
            }

            src_x     += count;
            dst_x     += count;
            remaining -= count;
        }
    }

    return EPD_OK;
}

static epd_err_t epd_gfx_canvas_draw_glyph_native_rot0(epd_gfx_canvas_t canvas, epd_gfx_glyph_t glyph,
    const epd_gfx_glyph_clip_t* clip, epd_gfx_color_t color)
{
    for (uint16_t y = 0U; y < clip->height; ++y) {
        uint16_t       src_y   = clip->src_y + y;
        const uint8_t* src_row = glyph->data + (uint32_t)src_y * epd_gfx_glyph_stride(glyph->width);
        uint8_t*       dst_row = canvas->buf_native + (uint32_t)(clip->dst_y + y) * canvas->buf_stride;

        uint16_t src_x     = clip->src_x;
        uint16_t dst_x     = clip->dst_x;
        uint16_t remaining = clip->width;
        while (remaining) {
            uint8_t dst_digit = (uint8_t)(dst_x & 1U);
            uint8_t count     = (uint8_t)EPD_MIN((uint16_t)(2U - dst_digit), remaining);
            uint8_t mask      = 0U;
            for (uint8_t i = 0U; i < count; ++i) {
                if (epd_gfx_mono_bit_at(src_row, (uint16_t)(src_x + i))) {
                    mask |= (uint8_t)((dst_digit + i) ? 0x0FU : 0xF0U);
                }
            }
            if (mask) {
                uint16_t index = dst_x / 2U;
                epd_gfx_native_set_pixel_impl(dst_row + index, mask, color);
            }

            src_x     += count;
            dst_x     += count;
            remaining -= count;
        }
    }

    return EPD_OK;
}

static epd_err_t epd_gfx_canvas_draw_glyph_mapped(epd_gfx_canvas_t canvas, epd_gfx_glyph_t glyph,
    const epd_gfx_glyph_clip_t* clip, epd_gfx_color_t color)
{
    uint16_t glyph_stride = (uint16_t)epd_gfx_glyph_stride(glyph->width);
    for (uint16_t y = 0U; y < clip->height; ++y) {
        const uint8_t* src_row = glyph->data + (uint32_t)(clip->src_y + y) * glyph_stride;

        for (uint16_t x = 0U; x < clip->width; ++x) {
            if (epd_gfx_mono_bit_at(src_row, (uint16_t)(clip->src_x + x))) {
                (void)epd_gfx_canvas_draw_pixel(canvas, (epd_gfx_point_t){
                    (uint16_t)(clip->dst_x + x + 1U),
                    (uint16_t)(clip->dst_y + y + 1U),
                }, color);
            }
        }
    }

    return EPD_OK;
}

epd_err_t epd_gfx_canvas_draw_glyph(epd_gfx_canvas_t canvas, epd_gfx_glyph_t glyph,
    epd_gfx_point_t point, epd_gfx_color_t color)
{
    if (!canvas || !glyph) {
        return EPD_ERR_INVALID_ARG;
    }
    if (!glyph->data || glyph->width == 0 || glyph->height == 0) {
        return EPD_OK;
    }

    epd_gfx_glyph_clip_t clip;
    uint16_t             lw = epd_gfx_canvas_get_logical_width(canvas);
    uint16_t             lh = epd_gfx_canvas_get_logical_height(canvas);
    if (!epd_gfx_canvas_clip_glyph(glyph, point.x, point.y, lw, lh, &clip)) {
        return EPD_OK;
    }

    if (epd_gfx_canvas_get_rotation(canvas) == EPD_GFX_ROTATE_0) {
        return epd_gfx_canvas_in_planes(canvas)
            ? epd_gfx_canvas_draw_glyph_planes_rot0(canvas, glyph, &clip, color)
            : epd_gfx_canvas_draw_glyph_native_rot0(canvas, glyph, &clip, color);
    }

    return epd_gfx_canvas_draw_glyph_mapped(canvas, glyph, &clip, color);
}
