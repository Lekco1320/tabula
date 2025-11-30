/**
 * @file canvas.c
 * @brief Graphics layer for 7.5\" tri-color e-paper (DEPG0750* UC8159).
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2025-11-21
 * @license MIT
 */

#include <string.h>
#include <epd_core/math.h>

#include "epd_gfx/canvas.h"

typedef void (*epd_gfx_map_fn_t)(const epd_gfx_canvas_t, uint16_t*, uint16_t*);
typedef uint16_t (*epd_gfx_size_fn_t)(const epd_gfx_canvas_t);

struct epd_gfx_canvas_impl {
    uint16_t           width;
    uint16_t           height;
    epd_gfx_rotation_t rotation;
    epd_gfx_map_fn_t   map_fn;
    epd_gfx_size_fn_t  lwidth_fn;
    epd_gfx_size_fn_t  lheight_fn;

    uint16_t           buf_stride;
    uint32_t           buf_size;
    uint8_t*           buf_native;
    uint8_t*           buf_wht;
    uint8_t*           buf_red;
};

static inline void swap(uint16_t* a, uint16_t* b)
{
    uint16_t temp = *a;
    *a = *b;
    *b = temp;
}

static inline bool epd_gfx_canvas_in_planes(const epd_gfx_canvas_t canvas)
{
    return !canvas->buf_native;
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

static inline bool epd_gfx_check_bound(const epd_gfx_canvas_t canvas, uint16_t x, uint16_t y)
{
    return x <= canvas->width && y <= canvas->height && x > 0 && y > 0;
}

static inline bool epd_gfx_check_bound_mapped(const epd_gfx_canvas_t canvas, uint16_t x, uint16_t y)
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
        canvas->buf_stride = (canvas->width + 1U) / 2U;
        canvas->buf_size   = (uint32_t)canvas->buf_stride * canvas->height;
        canvas->buf_native = (uint8_t*)calloc(canvas->buf_size, sizeof(uint8_t));
        if (!canvas->buf_native) {
            ret = EPD_ERR_NO_MEM;
            goto fail;
        }
        break;

    case EPD_GFX_FORMAT_PLANES:
        canvas->buf_stride = (canvas->width + 7U) / 8U;
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
    return (!canvas ? EPD_GFX_ROTATE_UNKNOWN : canvas->rotation);
}

epd_gfx_format_t epd_gfx_canvas_get_format(const epd_gfx_canvas_t canvas)
{
    return (!canvas ? EPD_GFX_FORMAT_UNKNOWN : (!canvas->buf_native));
}

epd_err_t epd_gfx_canvas_set_rotation(epd_gfx_canvas_t canvas, epd_gfx_rotation_t rotation)
{
    if (!canvas) {
        return EPD_ERR_INVALID_ARG;
    }

    switch (rotation)
    {
    case EPD_GFX_ROTATE_0:
        canvas->rotation   = EPD_GFX_ROTATE_0;
        canvas->map_fn     = epd_gfx_canvas_map_rot0;
        canvas->lwidth_fn  = epd_gfx_canvas_get_width;
        canvas->lheight_fn = epd_gfx_canvas_get_height;
        break;
    
    case EPD_GFX_ROTATE_90:
        canvas->rotation   = EPD_GFX_ROTATE_90;
        canvas->map_fn     = epd_gfx_canvas_map_rot90;
        canvas->lwidth_fn  = epd_gfx_canvas_get_height;
        canvas->lheight_fn = epd_gfx_canvas_get_width;
        break;

    case EPD_GFX_ROTATE_180:
        canvas->rotation   = EPD_GFX_ROTATE_180;
        canvas->map_fn     = epd_gfx_canvas_map_rot180;
        canvas->lwidth_fn  = epd_gfx_canvas_get_width;
        canvas->lheight_fn = epd_gfx_canvas_get_height;
        break;

    case EPD_GFX_ROTATE_270:
        canvas->rotation   = EPD_GFX_ROTATE_270;
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
        memset(canvas->buf_wht, (uint8_t)-(color & 1U), canvas->buf_size);
        memset(canvas->buf_red, (uint8_t)-((color >> 2) & 1U), canvas->buf_size);
    } else {
        uint8_t color_byte = ((color << 4) | (color & 0x0F));
        memset(canvas->buf_native, color_byte, canvas->buf_size);
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
        uint16_t index = canvas->buf_stride * y + (x >> 3);
        uint8_t  mask  = 1U << (7 - (x & 7));
        uint8_t  wold  = canvas->buf_wht[index];
        uint8_t  rold  = canvas->buf_red[index];
        uint8_t  wsel  = (uint8_t)-(color & 1U);
        uint8_t  rsel  = (uint8_t)-((color >> 2) & 1U);
        canvas->buf_wht[index] = (uint8_t)((wold & (uint8_t)~mask) | (wsel & mask));
        canvas->buf_red[index] = (uint8_t)((rold & (uint8_t)~mask) | (rsel & mask));
    } else {
        uint32_t index = canvas->buf_stride * y + (x >> 1);
        uint8_t  shift = 4 - ((x & 1) << 2);
        uint8_t  mask  = 15U << shift;
        uint8_t  old   = canvas->buf_native[index];
        uint8_t  sel   = color << shift;
        canvas->buf_native[index] = (uint8_t)((old & (uint8_t)~mask) | (sel & mask));
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
        uint8_t b0   = x >> 3;
        uint8_t b1   = x1 >> 3;
        uint8_t d0   = (uint8_t)x & 7U;
        uint8_t d1   = (uint8_t)x1 & 7U;
        uint8_t wsel = (uint8_t)-(color & 1U);
        uint8_t rsel = (uint8_t)-((color >> 2) & 1U);
        if (b0 == b1) {
            uint8_t  mask  = (uint8_t)((uint8_t)(0xFF >> d0) & (uint8_t)(0xFF << (7U - d1)));
            uint32_t index = canvas->buf_stride * y + b0;
            uint8_t  wold  = canvas->buf_wht[index];
            uint8_t  rold  = canvas->buf_red[index];
            canvas->buf_wht[index] = (uint8_t)((wold & (uint8_t)~mask) | (wsel & mask));
            canvas->buf_red[index] = (uint8_t)((rold & (uint8_t)~mask) | (rsel & mask));
            return EPD_OK;
        }
        if (d0) {
            uint8_t  mask  = (uint8_t)(0xFF >> d0);
            uint16_t index = canvas->buf_stride * y + b0;
            uint8_t  wold  = canvas->buf_wht[index];
            uint8_t  rold  = canvas->buf_red[index];
            canvas->buf_wht[index] = (uint8_t)((wold & (uint8_t)~mask) | (wsel & mask));
            canvas->buf_red[index] = (uint8_t)((rold & (uint8_t)~mask) | (rsel & mask));
            ++b0;
        }
        if (d1 < 7) {
            uint8_t  mask  = (uint8_t)(0xFF << (7U - d1));
            uint16_t index = canvas->buf_stride * y + b1;
            uint8_t  wold  = canvas->buf_wht[index];
            uint8_t  rold  = canvas->buf_red[index];
            canvas->buf_wht[index] = (uint8_t)((wold & (uint8_t)~mask) | (wsel & mask));
            canvas->buf_red[index] = (uint8_t)((rold & (uint8_t)~mask) | (rsel & mask));
            --b1;
        }
        if (b1 >= b0) {
            uint16_t len   = b1 - b0 + 1U;
            uint16_t index = canvas->buf_stride * y + b0;
            memset(canvas->buf_wht + index, wsel, len);
            memset(canvas->buf_red + index, rsel, len);
        }
    } else {
        uint16_t b0 = x >> 1;
        uint16_t b1 = x1 >> 1;
        uint8_t  d0 = (uint8_t)x & 1U;
        uint8_t  d1 = (uint8_t)x1 & 1U;
        if (b0 == b1) {
            uint16_t index = canvas->buf_stride * y + b0;
            uint8_t  old   = canvas->buf_native[index];
            uint8_t  byte  = (d0 != d1) ? (uint8_t)((color << 4) | (color & 0x0F))
                : (d0 && d1) ? (uint8_t)((old & 0xF0) | (color & 0x0F))
                : (uint8_t)((old & 0x0F) | ((color & 0x0F) << 4));

            canvas->buf_native[index] = byte;
            return EPD_OK;
        }
        if (d0) {
            uint16_t index = canvas->buf_stride * y + b0;
            uint8_t  old   = canvas->buf_native[index];
            canvas->buf_native[index] = (uint8_t)((old & 0xF0) | (color & 0x0F));
            ++b0;
        }
        if (!d1) {
            uint16_t index = canvas->buf_stride * y + b1;
            uint8_t  old   = canvas->buf_native[index];
            canvas->buf_native[index] = (uint8_t)((color << 4) | (old & 0x0F));
            --b1;
        }
        if (b1 >= b0) {
            uint16_t len   = b1 - b0 + 1U;
            uint16_t index = canvas->buf_stride * y + b0;
            uint8_t  byte  = (uint8_t)((color << 4) | (color & 0x0F));
            memset(canvas->buf_native + index, byte, len);
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
        uint16_t byte  = x >> 3;
        uint16_t index = canvas->buf_stride * y + byte;
        uint8_t  digit = (uint8_t)x & 7U;
        uint8_t  mask  = (uint8_t)(1U << (7 - digit));
        uint8_t  wsel  = (uint8_t)-(color & 1U);
        uint8_t  rsel  = (uint8_t)-((color >> 2) & 1U);
        for (uint16_t h = y; h <= y1; ++h) {
            uint8_t wold = canvas->buf_wht[index];
            uint8_t rold = canvas->buf_red[index];
            canvas->buf_wht[index] = (uint8_t)((wold & (uint8_t)~mask) | (wsel & mask));
            canvas->buf_red[index] = (uint8_t)((rold & (uint8_t)~mask) | (rsel & mask));
            index += canvas->buf_stride;
        }
    } else {
        uint16_t byte  = x >> 1;
        uint16_t index = canvas->buf_stride * y + byte;
        uint8_t  digit = (uint8_t)x & 1U;
        uint8_t  shift = 4U - (digit << 2U);
        uint8_t  mask  = 15U << shift;
        uint8_t  sel   = color << shift;
        for (uint16_t h = y; h <= y1; ++h) {
            uint8_t old = canvas->buf_native[index];
            canvas->buf_native[index] = (uint8_t)((old & (uint8_t)~mask) | (sel & mask));
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

    switch (canvas->rotation)
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

    switch (canvas->rotation)
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
    switch (canvas->rotation)
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