/**
 * @file canvas.c
 * @brief Canvas API for drawing into EPD buffers (native or planes).
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2025-11-21
 * @license MIT
 */

#include <string.h>
#include <epd_core/math.h>
#include <epd_core/common.h>

#include "epd_gfx/codec.h"
#include "epd_gfx/canvas.h"

typedef void (*epd_gfx_map_fn_t)(const epd_gfx_canvas_t, uint16_t*, uint16_t*);
typedef uint16_t (*epd_gfx_size_fn_t)(const epd_gfx_canvas_t);

struct epd_gfx_canvas_impl {
    uint16_t          width;
    uint16_t          height;
    uint8_t           flags;      // bit0=format (0 native / 1 planes), bits1-2=rotation (0/90/180/270)
    epd_gfx_map_fn_t  map_fn;
    epd_gfx_size_fn_t lwidth_fn;
    epd_gfx_size_fn_t lheight_fn;

    uint16_t          buf_stride;
    uint32_t          buf_size;
    union {
        struct {
            uint8_t*  buf_native;
        };
        struct {
            uint8_t*  buf_wht;
            uint8_t*  buf_red;
        };
    };
};

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
    *px = canvas->width - 1U - *py;
    *py = x;
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
    *px = *py;
    *py = canvas->height - 1U - x;
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

typedef epd_err_t (*epd_gfx_canvas_draw_line_impl)(epd_gfx_canvas_t, uint16_t,
    uint16_t, uint16_t, epd_gfx_color_t);

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
        return epd_gfx_canvas_draw_vline_impl(canvas, x, y, w, color);

    case EPD_GFX_ROTATE_180:
        x = epd_sat_sub_uint16(x, w - 1);
        return epd_gfx_canvas_draw_hline_impl(canvas, x, y, w, color);

    case EPD_GFX_ROTATE_270:
        y = epd_sat_sub_uint16(y, w - 1);
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
        y = epd_sat_sub_uint16(y, h - 1);
        return epd_gfx_canvas_draw_vline_impl(canvas, x, y, h, color);

    case EPD_GFX_ROTATE_270:
        x = epd_sat_sub_uint16(x, h - 1);
        return epd_gfx_canvas_draw_hline_impl(canvas, x, y, h, color);

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
    uint16_t  x1  = epd_sat_add_uint16(x, w - 1);
    uint16_t  y1  = epd_sat_add_uint16(y, h - 1);
    EPD_CHECK_RET(epd_gfx_canvas_draw_hline(canvas, x, y, w, color));
    EPD_CHECK_RET(epd_gfx_canvas_draw_vline(canvas, x, y, h, color));
    EPD_CHECK_RET(epd_gfx_canvas_draw_hline(canvas, x, y1, w, color));
    EPD_CHECK_RET(epd_gfx_canvas_draw_vline(canvas, x1, y, h, color));
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

    canvas->map_fn(canvas, &x, &y);
    if (!epd_gfx_check_bound_mapped(canvas, x, y)) {
        return EPD_ERR_INVALID_ARG;
    }
    
    uint16_t x0;
    uint16_t y0;
    uint16_t y1;
    uint16_t width;
    epd_gfx_rotation_t rotation = epd_gfx_canvas_get_rotation(canvas);
    switch (rotation)
    {
    case EPD_GFX_ROTATE_0:
        x0    = x;
        y0    = y;
        y1    = epd_sat_add_uint16(y, h - 1);
        width = w;
        break;
    
    case EPD_GFX_ROTATE_90:
        x0    = x;
        y0    = epd_sat_sub_uint16(y, w - 1);
        y1    = y;
        width = h;
        break;

    case EPD_GFX_ROTATE_180:
        x0    = epd_sat_sub_uint16(x, w - 1);
        y0    = epd_sat_sub_uint16(y, h - 1);
        y1    = y;
        width = w;
        break;

    case EPD_GFX_ROTATE_270:
        x0    = epd_sat_sub_uint16(y, h - 1);
        y0    = x;
        y1    = epd_sat_add_uint16(x, w - 1);
        width = h;
        break;

    default:
        return EPD_ERR_INVALID_ARG;
    }

    y1 = EPD_MIN(y1, canvas->height - 1);
    x0 = EPD_MIN(x0, canvas->width  - 1);
    y0 = EPD_MIN(y0, canvas->height - 1);

    epd_err_t ret = EPD_OK;
    for (uint16_t y = y0; y <= y1; ++y) {
        EPD_CHECK_RET(epd_gfx_canvas_draw_hline_impl(canvas, x0, y, width, color));
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