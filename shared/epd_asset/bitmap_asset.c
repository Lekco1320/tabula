/**
 * @file bitmap_asset.c
 * @brief Bitmap asset writer API implementation.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2026-05-17
 * @license MIT
 */

#include <stdint.h>
#include <epd_asset/bitmap_asset.h>
#include <epd_gfx/codec.h>
#include <epd_gfx/ebm.h>

static epd_err_t epd_asset_bitmap_write_exact(epd_stream_t* stream, const void* data, size_t size)
{
    return epd_stream_write_exact(stream, data, size) ? EPD_OK : EPD_ERR_INVALID_STATE;
}

static uint32_t epd_asset_bitmap_standard_stride(uint16_t width, epd_gfx_format_t format)
{
    switch (format) {
    case EPD_GFX_FORMAT_NATIVE:
        return epd_gfx_native_stride(width);

    case EPD_GFX_FORMAT_PLANES:
        return epd_gfx_planes_stride(width);

    default:
        return 0U;
    }
}

epd_err_t epd_asset_bitmap_write_ebm(const epd_gfx_frame_view_t* view,
    epd_stream_t* stream)
{
    if (!view || !stream) {
        return EPD_ERR_INVALID_ARG;
    }
    if (!epd_gfx_ebm_format_valid(view->format)) {
        return EPD_ERR_INVALID_ARG;
    }
    if (view->width == 0U || view->height == 0U) {
        return EPD_ERR_INVALID_SIZE;
    }

    uint32_t expected_stride = epd_asset_bitmap_standard_stride(view->width, view->format);
    if (expected_stride == 0U || view->stride != expected_stride) {
        return EPD_ERR_INVALID_SIZE;
    }

    uint32_t data_bytes = epd_gfx_ebm_data_bytes(view->width, view->height, view->format);
    if (data_bytes == 0U) {
        return EPD_ERR_INVALID_SIZE;
    }
    if (view->format == EPD_GFX_FORMAT_NATIVE && !view->buf_native) {
        return EPD_ERR_INVALID_ARG;
    }
    if (view->format == EPD_GFX_FORMAT_PLANES && (!view->buf_wht || !view->buf_red)) {
        return EPD_ERR_INVALID_ARG;
    }

    uint8_t   format = (uint8_t)view->format;
    epd_err_t ret    = EPD_OK;
    EPD_CHECK_RET(epd_asset_bitmap_write_exact(stream, EPD_GFX_EBM_MAGIC, EPD_GFX_EBM_MAGIC_BYTES));
    EPD_CHECK_RET(epd_asset_bitmap_write_exact(stream, &view->width, sizeof(view->width)));
    EPD_CHECK_RET(epd_asset_bitmap_write_exact(stream, &view->height, sizeof(view->height)));
    EPD_CHECK_RET(epd_asset_bitmap_write_exact(stream, &format, sizeof(format)));

    switch (view->format) {
    case EPD_GFX_FORMAT_NATIVE:
        return epd_asset_bitmap_write_exact(stream, view->buf_native, data_bytes);

    case EPD_GFX_FORMAT_PLANES:
        data_bytes /= 2U;
        EPD_CHECK_RET(epd_asset_bitmap_write_exact(stream, view->buf_wht, data_bytes));
        return epd_asset_bitmap_write_exact(stream, view->buf_red, data_bytes);

    default:
        return EPD_ERR_INVALID_ARG;
    }
}
