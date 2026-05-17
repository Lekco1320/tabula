/**
 * @file bitmap.c
 * @brief Custom bitmap API and storage implementation.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2026-05-17
 * @license MIT
 */

#include <stdlib.h>
#include <string.h>
#include <epd_gfx/bitmap.h>
#include <epd_gfx/codec.h>
#include <epd_gfx/ebm.h>

#include "epd_gfx/bitmap_impl.h"

static uint32_t epd_gfx_bitmap_buffer_bytes(uint16_t width, uint16_t height, epd_gfx_format_t format)
{
    if (width == 0U || height == 0U) {
        return 0U;
    }

    switch (format) {
    case EPD_GFX_FORMAT_NATIVE:
        return epd_gfx_native_stride(width) * height;

    case EPD_GFX_FORMAT_PLANES:
        return epd_gfx_planes_stride(width) * height;

    default:
        return 0U;
    }
}

static uint16_t epd_gfx_bitmap_stride(uint16_t width, epd_gfx_format_t format)
{
    switch (format) {
    case EPD_GFX_FORMAT_NATIVE:
        return (uint16_t)epd_gfx_native_stride(width);

    case EPD_GFX_FORMAT_PLANES:
        return (uint16_t)epd_gfx_planes_stride(width);

    default:
        return 0U;
    }
}

static epd_err_t epd_gfx_bitmap_load_native(const epd_stream_t* stream, epd_gfx_bitmap_t bitmap)
{
    bitmap->buf_native = (uint8_t*)malloc(bitmap->buffer_bytes);
    if (!bitmap->buf_native) {
        return EPD_ERR_NO_MEM;
    }
    if (!epd_stream_read_exact(stream, bitmap->buf_native, bitmap->buffer_bytes)) {
        return EPD_ERR_INVALID_RESPONSE;
    }

    return EPD_OK;
}

static epd_err_t epd_gfx_bitmap_load_planes(const epd_stream_t* stream, epd_gfx_bitmap_t bitmap)
{
    bitmap->buf_wht = (uint8_t*)malloc(bitmap->buffer_bytes);
    bitmap->buf_red = (uint8_t*)malloc(bitmap->buffer_bytes);
    if (!bitmap->buf_wht || !bitmap->buf_red) {
        return EPD_ERR_NO_MEM;
    }
    if (!epd_stream_read_exact(stream, bitmap->buf_wht, bitmap->buffer_bytes) ||
        !epd_stream_read_exact(stream, bitmap->buf_red, bitmap->buffer_bytes)) {
        return EPD_ERR_INVALID_RESPONSE;
    }

    return EPD_OK;
}

epd_err_t epd_gfx_bitmap_load(const epd_stream_t* stream, epd_gfx_bitmap_t* out_bitmap)
{
    if (!stream || !out_bitmap) {
        return EPD_ERR_INVALID_ARG;
    }
    *out_bitmap = NULL;

    epd_gfx_ebm_header_t header = { 0 };
    if (!epd_stream_seek(stream, 0, EPD_SEEK_SET)) {
        return EPD_ERR_INVALID_STATE;
    }
    if (!epd_gfx_ebm_read_header(stream, &header)) {
        return EPD_ERR_INVALID_RESPONSE;
    }
    if (!epd_gfx_ebm_check_magic(&header)) {
        return EPD_ERR_INVALID_VERSION;
    }
    if (!epd_gfx_ebm_format_valid(header.format)) {
        return EPD_ERR_INVALID_RESPONSE;
    }
    if (header.width == 0U || header.height == 0U) {
        return EPD_ERR_INVALID_SIZE;
    }

    epd_gfx_bitmap_t bitmap = (epd_gfx_bitmap_t)calloc(1, sizeof(struct epd_gfx_bitmap_impl));
    if (!bitmap) {
        return EPD_ERR_NO_MEM;
    }

    bitmap->width        = header.width;
    bitmap->height       = header.height;
    bitmap->format       = header.format;
    bitmap->stride       = epd_gfx_bitmap_stride(header.width, header.format);
    bitmap->buffer_bytes = epd_gfx_bitmap_buffer_bytes(header.width, header.height, header.format);
    if (bitmap->stride == 0U || bitmap->buffer_bytes == 0U ||
        epd_gfx_ebm_data_bytes(header.width, header.height, header.format) == 0U) {
        epd_gfx_bitmap_destroy(bitmap);
        return EPD_ERR_INVALID_SIZE;
    }

    epd_err_t ret = EPD_OK;
    switch (bitmap->format) {
    case EPD_GFX_FORMAT_NATIVE:
        ret = epd_gfx_bitmap_load_native(stream, bitmap);
        break;

    case EPD_GFX_FORMAT_PLANES:
        ret = epd_gfx_bitmap_load_planes(stream, bitmap);
        break;

    default:
        ret = EPD_ERR_INVALID_RESPONSE;
        break;
    }
    if (ret != EPD_OK) {
        epd_gfx_bitmap_destroy(bitmap);
        return ret;
    }

    *out_bitmap = bitmap;
    return EPD_OK;
}

epd_err_t epd_gfx_bitmap_destroy(epd_gfx_bitmap_t bitmap)
{
    if (bitmap) {
        if (bitmap->format == EPD_GFX_FORMAT_PLANES) {
            free(bitmap->buf_wht);
            free(bitmap->buf_red);
        } else {
            free(bitmap->buf_native);
        }
        free(bitmap);
    }

    return EPD_OK;
}

uint16_t epd_gfx_bitmap_get_width(const epd_gfx_bitmap_t bitmap)
{
    return bitmap ? bitmap->width : 0U;
}

uint16_t epd_gfx_bitmap_get_height(const epd_gfx_bitmap_t bitmap)
{
    return bitmap ? bitmap->height : 0U;
}

epd_gfx_format_t epd_gfx_bitmap_get_format(const epd_gfx_bitmap_t bitmap)
{
    return bitmap ? bitmap->format : EPD_GFX_FORMAT_UNKNOWN;
}

epd_err_t epd_gfx_bitmap_get_frame_view(const epd_gfx_bitmap_t bitmap,
    epd_gfx_frame_view_t* out_view)
{
    if (!bitmap || !out_view) {
        return EPD_ERR_INVALID_ARG;
    }
    if (!epd_gfx_ebm_format_valid(bitmap->format)) {
        return EPD_ERR_INVALID_STATE;
    }

    memset(out_view, 0, sizeof(*out_view));
    out_view->format = bitmap->format;
    out_view->width  = bitmap->width;
    out_view->height = bitmap->height;
    out_view->stride = bitmap->stride;
    if (bitmap->format == EPD_GFX_FORMAT_PLANES) {
        out_view->buf_wht = bitmap->buf_wht;
        out_view->buf_red = bitmap->buf_red;
    } else {
        out_view->buf_native = bitmap->buf_native;
    }

    return EPD_OK;
}
