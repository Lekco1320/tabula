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
    uint16_t x, uint16_t y)
{
    if (!canvas) {
        return EPD_GFX_WHITE;
    }

    canvas->map_fn(canvas, &x, &y);
    if (!epd_gfx_check_bound_mapped(canvas, x, y)) {
        return EPD_GFX_WHITE;
    }

    if (epd_gfx_canvas_in_planes(canvas)) {
        uint32_t index = (uint32_t)canvas->buf_stride * y + x / 8U;
        uint8_t  digit = x % 8U;
        return epd_gfx_planes_get_pixel(canvas->buf_wht[index], canvas->buf_red[index], digit);
    } else {
        uint32_t index = (uint32_t)canvas->buf_stride * y + x / 2U;
        uint8_t  digit = x % 2U;
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
    uint16_t x, uint16_t y, epd_gfx_color_t color)
{
    if (!canvas) {
        return EPD_ERR_INVALID_ARG;
    }

    canvas->map_fn(canvas, &x, &y);
    if (!epd_gfx_check_bound_mapped(canvas, x, y)) {
        return EPD_ERR_INVALID_ARG;
    }

    if (epd_gfx_canvas_in_planes(canvas)) {
        uint16_t index = canvas->buf_stride * y + x / 8U;
        uint8_t  digit = x % 8;
        epd_gfx_planes_set_pixel(canvas->buf_wht + index, canvas->buf_red + index, digit, color);
    } else {
        uint32_t index = canvas->buf_stride * y + x / 2U;
        uint8_t  digit = x % 2;
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
    uint16_t x, uint16_t y, uint16_t w, epd_gfx_color_t color)
{
    if (!canvas) {
        return EPD_ERR_INVALID_ARG;
    }
    if (!w) {
        return EPD_OK;
    }
    if (w == 1) {
        return epd_gfx_canvas_draw_pixel(canvas, x, y, color);
    }

    canvas->map_fn(canvas, &x, &y);
    if (!epd_gfx_check_bound_mapped(canvas, x, y)) {
        return EPD_ERR_INVALID_ARG;
    }

    epd_gfx_rotation_t rotation = epd_gfx_canvas_get_rotation(canvas);
    switch (rotation)
    {
    case EPD_GFX_ROTATE_0:
        return epd_gfx_canvas_draw_hline_impl(canvas, x, y, w, color);    

    case EPD_GFX_ROTATE_90:
        uint16_t y0 = epd_sat_sub_uint16(y, w - 1);
        uint16_t hh = y - y0 + 1;
        return epd_gfx_canvas_draw_vline_impl(canvas, x, y0, hh, color);

    case EPD_GFX_ROTATE_180:
        uint16_t x0 = epd_sat_sub_uint16(x, w - 1);
        uint16_t ww = x - x0 + 1;
        return epd_gfx_canvas_draw_hline_impl(canvas, x0, y, ww, color);

    case EPD_GFX_ROTATE_270:
        return epd_gfx_canvas_draw_vline_impl(canvas, x, y, w, color);

    default:
        return EPD_ERR_INVALID_ARG;
    }
}

epd_err_t epd_gfx_canvas_draw_vline(epd_gfx_canvas_t canvas,
    uint16_t x, uint16_t y, uint16_t h, epd_gfx_color_t color)
{
    if (!canvas) {
        return EPD_ERR_INVALID_ARG;
    }
    if (!h) {
        return EPD_OK;
    }
    if (h == 1) {
        return epd_gfx_canvas_draw_pixel(canvas, x, y, color);
    }

    canvas->map_fn(canvas, &x, &y);
    if (!epd_gfx_check_bound_mapped(canvas, x, y)) {
        return EPD_ERR_INVALID_ARG;
    }

    epd_gfx_rotation_t rotation = epd_gfx_canvas_get_rotation(canvas);
    switch (rotation)
    {
    case EPD_GFX_ROTATE_0:
        return epd_gfx_canvas_draw_vline_impl(canvas, x, y, h, color);

    case EPD_GFX_ROTATE_90:
        return epd_gfx_canvas_draw_hline_impl(canvas, x, y, h, color);

    case EPD_GFX_ROTATE_180:
        uint16_t y0 = epd_sat_sub_uint16(y, h - 1);
        uint16_t hh = y - y0 + 1;
        return epd_gfx_canvas_draw_vline_impl(canvas, x, y0, hh, color);

    case EPD_GFX_ROTATE_270:
        uint16_t x0 = epd_sat_sub_uint16(x, h - 1);
        uint16_t ww = x - x0 + 1;
        return epd_gfx_canvas_draw_hline_impl(canvas, x0, y, ww, color);

    default:
        return EPD_ERR_INVALID_ARG;
    }
}

epd_err_t epd_gfx_canvas_draw_rect(epd_gfx_canvas_t canvas,
    uint16_t x, uint16_t y, uint16_t w, uint16_t h, epd_gfx_color_t color)
{
    if (!canvas) {
        return EPD_ERR_INVALID_ARG;
    }
    if (!w || !h) {
        return EPD_OK;
    }

    epd_err_t ret = EPD_OK;

    // Draw Top Edge
    EPD_CHECK_RET(epd_gfx_canvas_draw_hline(canvas, x, y, w, color));

    // Draw Bottom Edge
    uint16_t lw = epd_gfx_canvas_get_logical_width(canvas);
    uint16_t lh = epd_gfx_canvas_get_logical_height(canvas);
    uint16_t x1 = epd_sat_add_uint16(x, w - 1);
    uint16_t y1 = epd_sat_add_uint16(y, h - 1);
    if (h > 1 && y1 <= lh) {
        EPD_CHECK_RET(epd_gfx_canvas_draw_hline(canvas, x, y1, w, color));
    }

    // Draw Side Edges (excluding corners)
    if (h > 2) {
        uint16_t y_inner = epd_sat_add_uint16(y, 1);
        uint16_t h_inner = h - 2;
        if (y_inner <= lh) {
            EPD_CHECK_RET(epd_gfx_canvas_draw_vline(canvas, x, y_inner, h_inner, color));
            if (w > 1 && x1 <= lw) {
                EPD_CHECK_RET(epd_gfx_canvas_draw_vline(canvas, x1, y_inner, h_inner, color));
            }
        }
    }

    return EPD_OK;
}

epd_err_t epd_gfx_canvas_fill_rect(epd_gfx_canvas_t canvas,
    uint16_t x, uint16_t y, uint16_t w, uint16_t h, epd_gfx_color_t color)
{
    if (!canvas) {
        return EPD_ERR_INVALID_ARG;
    }
    if (!w || !h) {
        return EPD_OK;
    }

    uint16_t px = x;
    uint16_t py = y;
    canvas->map_fn(canvas, &px, &py);
    
    uint16_t x0, y0, x1, y1;
    epd_gfx_rotation_t rotation = epd_gfx_canvas_get_rotation(canvas);
    switch (rotation)
    {
    case EPD_GFX_ROTATE_0:
        x0 = px;
        y0 = py;
        x1 = epd_sat_add_uint16(px, w - 1);
        y1 = epd_sat_add_uint16(py, h - 1);
        break;
    
    case EPD_GFX_ROTATE_90:
        x0 = px;
        y0 = epd_sat_sub_uint16(py, w - 1);
        x1 = epd_sat_add_uint16(px, h - 1);
        y1 = py;
        break;

    case EPD_GFX_ROTATE_180:
        x0 = epd_sat_sub_uint16(px, w - 1);
        y0 = epd_sat_sub_uint16(py, h - 1);
        x1 = px;
        y1 = py;
        break;

    case EPD_GFX_ROTATE_270:
        x0 = epd_sat_sub_uint16(px, h - 1);
        y0 = py;
        x1 = px;
        y1 = epd_sat_add_uint16(py, w - 1);
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

epd_err_t epd_gfx_canvas_draw_glyph(epd_gfx_canvas_t canvas, epd_gfx_glyph_t glyph,
    uint16_t x, uint16_t y, epd_gfx_color_t color, epd_gfx_bg_color_t background)
{
    if (!canvas || !glyph) {
        return EPD_ERR_INVALID_ARG;
    }
    if (!glyph->data || glyph->width == 0 || glyph->height == 0) {
        return EPD_OK;
    }

    epd_err_t ret = EPD_OK;
    if (background != EPD_GFX_BG_TRANSPARENT) {
        EPD_CHECK_RET(epd_gfx_canvas_fill_rect(canvas, x, y, glyph->advance, glyph->line_height,
            (epd_gfx_color_t)background));
    }

    int16_t  x0   = (int16_t)x + glyph->xoffset;
    int16_t  y0   = (int16_t)y + glyph->ascent + glyph->yoffset;
    uint16_t wb   = (glyph->width + 7U) / 8U;
    uint16_t lw   = epd_gfx_canvas_get_logical_width(canvas);
    uint16_t lh   = epd_gfx_canvas_get_logical_height(canvas);
    uint8_t* data = glyph->data;
    for (uint16_t h = 0; h < glyph->height; ++h) {
        for (uint16_t w = 0; w < wb; ++w) {
            uint8_t byte = *data++;
            if (!byte) {
                continue;
            }

            uint16_t base = w * 8U;
            for (uint8_t i = 0; i < 8 && (base + i) < glyph->width; ++i) {
                if (byte & (uint8_t)(0x80U >> i)) {
                    int16_t px = x0 + base + i;
                    int16_t py = y0 + h;
                    if (px < 0 || py < 0) {
                        continue;
                    }
                    if ((uint16_t)px >= lw || (uint16_t)py >= lh) {
                        continue;
                    }
                    (void)epd_gfx_canvas_draw_pixel(canvas, (uint16_t)px, (uint16_t)py, color);
                }
            }
        }
    }

    return EPD_OK;
}
