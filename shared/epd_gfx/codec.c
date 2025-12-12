/**
 * @file codec.c
 * @brief Pixel format codec and pixel operations for EPD buffers.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2025-12-6
 * @license MIT
 */

#include <epd_core/common.h>

#include "epd_gfx/codec.h"

epd_err_t epd_gfx_planes_to_native_buffer(const uint8_t* pwht, const uint8_t* pred,
    uint32_t width, uint8_t* out_native)
{
    if (!pwht || !pred || !out_native) {
        return EPD_ERR_INVALID_ARG;
    }

    uint32_t native_stride = epd_gfx_native_stride(width);
    for (uint32_t idx = 0; idx < native_stride; ++idx) {
        uint32_t pidx   = idx / 4U;
        uint8_t  digit  = 7U - (idx % 4U) * 2U;
        uint8_t  c0     = epd_gfx_bit_to_color(epd_gfx_bit_at(pwht[pidx], digit - 0),
            epd_gfx_bit_at(pred[pidx], digit - 0));
        uint8_t  c1     = epd_gfx_bit_to_color(epd_gfx_bit_at(pwht[pidx], digit - 1),
            epd_gfx_bit_at(pred[pidx], digit - 1));
        out_native[idx] = epd_gfx_pack_colors(c0, c1);
    }

    return EPD_OK;
}

epd_err_t epd_gfx_planes_to_color_buffer(const uint8_t* pwht, const uint8_t* pred,
    uint32_t width, epd_gfx_color_t* out_color)
{
    if (!pwht || !pred || !out_color) {
        return EPD_ERR_INVALID_ARG;
    }

    for (uint32_t idx = 0; idx < width; ++idx) {
        uint32_t pidx  = idx / 8U;
        uint8_t  digit = 7U - (idx % 8U);
        uint8_t  wbit  = epd_gfx_bit_at(pwht[pidx], digit);
        uint8_t  rbit  = epd_gfx_bit_at(pred[pidx], digit);
        out_color[idx] = epd_gfx_bit_to_color(wbit, rbit);
    }

    return EPD_OK;
}

epd_err_t epd_gfx_native_to_planes_buffer(const uint8_t* pnative, uint32_t width,
    uint8_t* out_wht, uint8_t* out_red)
{
    if (!pnative || !out_wht || !out_red) {
        return EPD_ERR_INVALID_ARG;
    }

    uint32_t planes_stride = epd_gfx_planes_stride(width);
    for (uint32_t idx = 0; idx < planes_stride; ++idx) {
        uint32_t       nidx = idx * 4U;
        uint8_t*       pwht = out_wht + idx;
        uint8_t*       pred = out_red + idx;
        const uint8_t* pnat = pnative + nidx;
        for (uint8_t i = 0; i < 8 && idx * 8 + i < width; ++i) {
            uint8_t         nibble = (i + 1) % 2U;
            uint8_t         byte   = pnat[i / 2U];
            epd_gfx_color_t color  = epd_gfx_normalize_color(epd_gfx_nibble_at(byte, nibble));
            epd_gfx_planes_set_pixel(pwht, pred, i, color);
        }
    }

    return EPD_OK;
}

epd_err_t epd_gfx_native_to_color_buffer(const uint8_t* pnative, uint32_t width,
    epd_gfx_color_t* out_color)
{
    if (!pnative || !out_color) {
        return EPD_ERR_INVALID_ARG;
    }

    for (uint32_t idx = 0; idx < width; ++idx) {
        uint32_t nidx  = idx / 2U;
        uint8_t  digit = 1U - (idx % 2U);
        out_color[idx] = epd_gfx_normalize_color(epd_gfx_nibble_at(pnative[nidx], digit));
    }

    return EPD_OK;
}

epd_err_t epd_gfx_color_to_native_buffer(const epd_gfx_color_t* pcolor, uint32_t width,
    uint8_t* out_native)
{
    if (!pcolor || !out_native) {
        return EPD_ERR_INVALID_ARG;
    }

    uint32_t native_stride = epd_gfx_native_stride(width);
    for (uint32_t idx = 0; idx < native_stride; ++idx) {
        uint32_t        cidx = idx * 2U;
        epd_gfx_color_t c1   = pcolor[cidx];
        epd_gfx_color_t c2   = ((cidx + 1 < width) ? pcolor[cidx + 1] : EPD_GFX_WHITE);
        out_native[idx]      = epd_gfx_pack_colors(c1, c2);
    }

    return EPD_OK;
}

epd_err_t epd_gfx_color_to_planes_buffer(const epd_gfx_color_t* pcolor, uint32_t width,
    uint8_t* out_wht, uint8_t* out_red)
{
    if (!pcolor || !out_wht || !out_red) {
        return EPD_ERR_INVALID_ARG;
    }

    uint32_t planes_stride = epd_gfx_planes_stride(width);
    for (uint32_t idx = 0; idx < planes_stride; ++idx) {
        for (uint8_t i = 0; i < 4; ++i) {
            uint8_t  digit = i * 2U;
            uint32_t cidx  = idx * 8U + digit;
            epd_gfx_planes_set_pixel(out_wht + idx, out_red + idx, digit, pcolor[cidx]);
            if (++cidx >= width) {
                return EPD_OK;
            }
            epd_gfx_planes_set_pixel(out_wht + idx, out_red + idx, digit + 1, pcolor[cidx]);
        }
    }

    return EPD_OK;
}
