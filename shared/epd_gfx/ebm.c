/**
 * @file ebm.c
 * @brief EBM1 bitmap file format helper implementation.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2026-05-17
 * @license MIT
 */

#include <stdint.h>
#include <epd_gfx/codec.h>
#include <epd_gfx/ebm.h>

bool epd_gfx_ebm_check_magic(const epd_gfx_ebm_header_t* header)
{
    if (!header) {
        return false;
    }

    return header->magic[0] == EPD_GFX_EBM_MAGIC[0] &&
           header->magic[1] == EPD_GFX_EBM_MAGIC[1] &&
           header->magic[2] == EPD_GFX_EBM_MAGIC[2] &&
           header->magic[3] == EPD_GFX_EBM_MAGIC[3];
}

bool epd_gfx_ebm_format_valid(epd_gfx_format_t format)
{
    return format == EPD_GFX_FORMAT_NATIVE || format == EPD_GFX_FORMAT_PLANES;
}

uint32_t epd_gfx_ebm_data_bytes(uint16_t width, uint16_t height, epd_gfx_format_t format)
{
    if (width == 0U || height == 0U) {
        return 0U;
    }

    switch (format) {
    case EPD_GFX_FORMAT_NATIVE:
        return epd_gfx_native_stride(width) * height;

    case EPD_GFX_FORMAT_PLANES:
        return epd_gfx_planes_stride(width) * height * 2U;

    default:
        return 0U;
    }
}

bool epd_gfx_ebm_read_header(const epd_stream_t* stream, epd_gfx_ebm_header_t* header)
{
    if (!header) {
        return false;
    }

    uint8_t format = EPD_GFX_FORMAT_UNKNOWN;
    if (!epd_stream_read_exact(stream, header->magic, sizeof(header->magic)) ||
        !epd_stream_read_exact(stream, &header->width, sizeof(header->width)) ||
        !epd_stream_read_exact(stream, &header->height, sizeof(header->height)) ||
        !epd_stream_read_exact(stream, &format, sizeof(format))) {
        return false;
    }

    header->format = (epd_gfx_format_t)format;
    return true;
}
