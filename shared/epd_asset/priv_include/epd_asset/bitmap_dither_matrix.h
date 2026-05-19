/**
 * @file bitmap_dither_matrix.h
 * @brief Declarations for precomputed dithering threshold matrices.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2026-05-19
 * @license MIT
 */

#pragma once

#ifndef _EPD_ASSET_BITMAP_DITHER_MATRIX_H_
#define _EPD_ASSET_BITMAP_DITHER_MATRIX_H_

#include <stdint.h>

#define EPD_ASSET_BITMAP_DITHER_MATRIX_BLUE_NOISE_64_SIZE  64U
#define EPD_ASSET_BITMAP_DITHER_MATRIX_BLUE_NOISE_128_SIZE 128U
#define EPD_ASSET_BITMAP_DITHER_MATRIX_BLUE_NOISE_256_SIZE 256U

extern const uint8_t EPD_ASSET_BITMAP_DITHER_MATRIX_ORDERED_2X2[2][2];
extern const uint8_t EPD_ASSET_BITMAP_DITHER_MATRIX_ORDERED_4X4[4][4];
extern const uint8_t EPD_ASSET_BITMAP_DITHER_MATRIX_ORDERED_8X8[8][8];

extern const uint8_t EPD_ASSET_BITMAP_DITHER_MATRIX_BLUE_NOISE_64X64[64][64];
extern const uint8_t EPD_ASSET_BITMAP_DITHER_MATRIX_BLUE_NOISE_128X128[128][128];
extern const uint8_t EPD_ASSET_BITMAP_DITHER_MATRIX_BLUE_NOISE_256X256[256][256];

#endif // !_EPD_ASSET_BITMAP_DITHER_MATRIX_H_
