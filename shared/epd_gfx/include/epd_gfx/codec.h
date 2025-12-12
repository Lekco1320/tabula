/**
 * @file codec.h
 * @brief Pixel format codec and pixel operations for EPD buffers.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2025-12-6
 * @license MIT
 */

#pragma once

#ifndef _EPD_GFX_CODEC_H_
#define _EPD_GFX_CODEC_H_

#include <stdint.h>
#include <string.h>
#include <epd_core/common.h>

#include "epd_gfx/common.h"

#ifdef __cplusplus
extern "C" {
#endif

static EPD_INLINE uint32_t epd_gfx_native_stride(uint32_t width)
{
    return (width + 1U) / 2U;
}

static EPD_INLINE uint32_t epd_gfx_planes_stride(uint32_t width)
{
    return (width + 7U) / 8U;
}

static EPD_INLINE uint8_t epd_gfx_bit_at(uint8_t byte, uint8_t digit)
{
    return (uint8_t)((byte >> digit) & 1U);    // x: 0..7
}

static EPD_INLINE uint8_t epd_gfx_nibble_at(uint8_t byte, uint8_t digit)
{
    return (uint8_t)((byte >> (digit << 2U)) & 0x0F);
}

static EPD_INLINE uint8_t epd_gfx_mask_bit(uint8_t digit)
{
    return (uint8_t)(1U << digit);            // x: 0..7
}

static EPD_INLINE uint8_t epd_gfx_mask_high_bits(uint8_t count)
{
    return (uint8_t)(0xFF << (8U - count));   // x: 0..8
}

static EPD_INLINE uint8_t epd_gfx_mask_low_bits(uint8_t count)
{
    return (uint8_t)(0xFF >> (8U - count));   // x: 0..8
}

static EPD_INLINE uint8_t epd_gfx_mask_range_bits(uint8_t begin, uint8_t end)
{
    return (uint8_t)((uint8_t)(0xFF << begin) & (uint8_t)(0xFF >> (7U - end)));
}

static EPD_INLINE uint8_t epd_gfx_mask_nibble(uint8_t digit)
{
    return (uint8_t)(0x0F << (digit << 2U));  // x: 0..1
}

static EPD_INLINE epd_gfx_color_t epd_gfx_bit_to_color(uint8_t wbit, uint8_t rbit)
{
    uint8_t t = (1 ^ rbit) & wbit;
    uint8_t w = (t << 1) | t;
    uint8_t r = (rbit << 2);
    return (epd_gfx_color_t)(r | w);
}

static EPD_INLINE void epd_gfx_color_to_bits(epd_gfx_color_t color, uint8_t* wbit, uint8_t* rbit)
{
    *rbit = (color >> 2) & 1;
    *wbit = !((color & 3) ^ 3);
}

static EPD_INLINE void epd_gfx_color_to_byte(epd_gfx_color_t color, uint8_t* wbyte, uint8_t* rbyte)
{
    *wbyte = (uint8_t)-(color & 1U);
    *rbyte = (uint8_t)-((color >> 2) & 1U);
}

static EPD_INLINE uint8_t epd_gfx_pack_colors(epd_gfx_color_t first, epd_gfx_color_t second)
{
    return (first << 4) | (second & 0x0F);
}

static EPD_INLINE epd_gfx_color_t epd_gfx_normalize_color(uint8_t color)
{
    uint8_t b0 = color & 1;
    uint8_t b1 = (color >> 1) & 1;
    uint8_t b2 = (color >> 2) & 1;
    uint8_t t  = b0 | b1;
    return (epd_gfx_color_t)((uint8_t)((b2 & ~(b1 | b0)) << 2) | (t << 1) | t);
}

static EPD_INLINE void epd_gfx_planes_set_pixel_impl(uint8_t* pwht, uint8_t* pred,
    uint8_t mask, epd_gfx_color_t color)
{
    uint8_t wold = *pwht;
    uint8_t rold = *pred;
    uint8_t wbyte, rbyte;
    epd_gfx_color_to_byte(color, &wbyte, &rbyte);
    *pwht = (uint8_t)((wold & (uint8_t)~mask) | (wbyte & mask));
    *pred = (uint8_t)((rold & (uint8_t)~mask) | (rbyte & mask));
}

static EPD_INLINE void epd_gfx_planes_set_pixel(uint8_t* pwht, uint8_t* pred,
    uint8_t idx, epd_gfx_color_t color)
{
    uint8_t digit = 7U - (idx & 7U);
    uint8_t mask  = epd_gfx_mask_bit(digit);
    epd_gfx_planes_set_pixel_impl(pwht, pred, mask, color);
}

static EPD_INLINE void epd_gfx_planes_set_range_pixels(uint8_t* pwht, uint8_t* pred,
    uint8_t begin, uint8_t end, epd_gfx_color_t color)
{
    uint8_t d0   = 7U - (end & 7U);
    uint8_t d1   = 7U - (begin & 7U);
    uint8_t mask = epd_gfx_mask_range_bits(d0, d1);
    epd_gfx_planes_set_pixel_impl(pwht, pred, mask, color);
}

static EPD_INLINE void epd_gfx_planes_set_bytes(uint8_t* pwht, uint8_t* pred,
    uint32_t length, epd_gfx_color_t color)
{
    uint8_t wbyte, rbyte;
    epd_gfx_color_to_byte(color, &wbyte, &rbyte);
    memset(pwht, wbyte, length);
    memset(pred, rbyte, length);
}

static EPD_INLINE void epd_gfx_native_set_pixel(uint8_t* pnative, uint8_t idx,
    epd_gfx_color_t color)
{
    uint8_t digit = 1U - (idx & 1U);
    uint8_t mask  = epd_gfx_mask_nibble(digit);
    uint8_t byte  = epd_gfx_pack_colors(color, color);
    uint8_t old   = *pnative;
    *pnative      = (uint8_t)((old & (uint8_t)~mask) | (byte & mask));
}

static EPD_INLINE void epd_gfx_native_set_bytes(uint8_t* pnative, uint32_t length,
    epd_gfx_color_t color)
{
    uint8_t byte = epd_gfx_pack_colors(color, color);
    memset(pnative, byte, length);
}

/**
 * @brief Convert white/red planes into a native buffer.
 *
 * @param pwht Pointer to the white plane buffer.
 * @param pred Pointer to the red plane buffer.
 * @param width Line width in pixels.
 * @param out_native Output native buffer with at least `width` elements.
 * @return `EPD_OK` on success, otherwise an error code from `epd_err_t`.
 */
epd_err_t epd_gfx_planes_to_native_buffer(const uint8_t* pwht, const uint8_t* pred,
    uint32_t width, uint8_t* out_native);

/**
 * @brief Convert white/red planes into a color buffer.
 *
 * @param pwht Pointer to the white plane buffer.
 * @param pred Pointer to the red plane buffer.
 * @param width Line width in pixels.
 * @param out_color Output color buffer with at least `width` elements.
 * @return `EPD_OK` on success, otherwise an error code from `epd_err_t`.
 */
epd_err_t epd_gfx_planes_to_color_buffer(const uint8_t* pwht, const uint8_t* pred,
    uint32_t width, epd_gfx_color_t* out_color);

/**
 * @brief Convert a native buffer into white/red planes.
 *
 * @param pnative Pointer to native buffer.
 * @param width Line width in pixels.
 * @param out_wht Output white plane buffer with at least `width` elements.
 * @param out_red Output red plane buffer with at least `width` elements.
 * @return `EPD_OK` on success, otherwise an error code from `epd_err_t`.
 */
epd_err_t epd_gfx_native_to_planes_buffer(const uint8_t* pnative, uint32_t width,
    uint8_t* out_wht, uint8_t* out_red);

/**
 * @brief Convert a native buffer into a color buffer.
 *
 * @param pnative Pointer to native buffer.
 * @param width Line width in pixels.
 * @param out_color Output color buffer with at least `width` elements.
 * @return `EPD_OK` on success, otherwise an error code from `epd_err_t`.
 */
epd_err_t epd_gfx_native_to_color_buffer(const uint8_t* pnative, uint32_t width,
    epd_gfx_color_t* out_color);

/**
 * @brief Convert a color buffer into a native buffer.
 *
 * @param pcolor Pointer to color buffer.
 * @param width Line width in pixels.
 * @param out_native Output native buffer with at least `width` elements.
 * @return `EPD_OK` on success, otherwise an error code from `epd_err_t`.
 */
epd_err_t epd_gfx_color_to_native_buffer(const epd_gfx_color_t* pcolor, uint32_t width,
    uint8_t* out_native);

/**
 * @brief Convert a color buffer into white/red planes.
 *
 * @param pcolor Pointer to color buffer.
 * @param width Line width in pixels.
 * @param out_wht Output white plane buffer with at least `width` elements.
 * @param out_red Output red plane buffer with at least `width` elements.
 * @return `EPD_OK` on success, otherwise an error code from `epd_err_t`.
 */
epd_err_t epd_gfx_color_to_planes_buffer(const epd_gfx_color_t* pcolor, uint32_t width,
    uint8_t* out_wht, uint8_t* out_red);

#ifdef __cplusplus
}
#endif

#endif // !_EPD_GFX_CODEC_H_
