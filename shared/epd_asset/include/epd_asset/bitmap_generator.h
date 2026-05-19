/**
 * @file bitmap_generator.h
 * @brief Bitmap generation API for converting RGB888 images to EPD frame buffers.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2026-05-18
 * @license MIT
 */

#pragma once

#ifndef _EPD_ASSET_BITMAP_GENERATOR_H_
#define _EPD_ASSET_BITMAP_GENERATOR_H_

#include <stdint.h>

#include <epd_core/common.h>
#include <epd_gfx/common.h>
#include <epd_gfx/frame_view.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const uint8_t* data;    /*!< RGB888 source pixels. */
    uint16_t       width;   /*!< Source width in pixels. */
    uint16_t       height;  /*!< Source height in pixels. */
    uint32_t       stride;  /*!< Source row stride in bytes. */
} epd_asset_bitmap_source_t;

typedef struct {
    epd_gfx_format_t format;            /*!< Output frame format. */
    uint8_t          black_threshold;   /*!< Luma threshold below which pixels become black. */
    uint8_t          red_threshold;     /*!< Minimum red channel advantage over green/blue. */
    uint8_t          red_saturation;    /*!< Minimum RGB saturation required to output red. */
} epd_asset_bitmap_threshold_config_t;

/**
 * @brief Generate an EPD frame view from an RGB888 image using threshold quantization.
 *
 * The function allocates output buffers and stores them in `out_view`. The caller
 * owns those buffers and must release them itself.
 *
 * @param source RGB888 source image.
 * @param config Threshold generation configuration.
 * @param out_view Pointer to receive the generated frame view.
 * @return `EPD_OK` on success, otherwise an error code from `epd_err_t`.
 */
epd_err_t epd_asset_bitmap_generate_threshold(const epd_asset_bitmap_source_t* source,
    const epd_asset_bitmap_threshold_config_t* config, epd_gfx_frame_view_t* out_view);

/**
 * @brief Ordered dithering threshold matrix size.
 */
typedef enum {
    EPD_ASSET_BITMAP_ORDERED_MATRIX_2X2,  /*!< 2x2 Bayer matrix. */
    EPD_ASSET_BITMAP_ORDERED_MATRIX_4X4,  /*!< 4x4 Bayer matrix. */
    EPD_ASSET_BITMAP_ORDERED_MATRIX_8X8,  /*!< 8x8 Bayer matrix. */
} epd_asset_bitmap_ordered_matrix_size_t;

/**
 * @brief Ordered dithering generation configuration.
 */
typedef struct {
    epd_gfx_format_t                       format;       /*!< Output frame format. */
    epd_asset_bitmap_ordered_matrix_size_t matrix_size;  /*!< Ordered dithering threshold matrix size. */
} epd_asset_bitmap_ordered_config_t;

/**
 * @brief Generate an EPD frame view from an RGB888 image using ordered dithering.
 *
 * The function allocates output buffers and stores them in `out_view`. The caller
 * owns those buffers and must release them itself.
 *
 * @param source RGB888 source image.
 * @param config Ordered dithering generation configuration.
 * @param out_view Pointer to receive the generated frame view.
 * @return `EPD_OK` on success, otherwise an error code from `epd_err_t`.
 */
epd_err_t epd_asset_bitmap_generate_ordered(const epd_asset_bitmap_source_t* source,
    const epd_asset_bitmap_ordered_config_t* config, epd_gfx_frame_view_t* out_view);

/**
 * @brief Blue-noise dithering threshold matrix size.
 */
typedef enum {
    EPD_ASSET_BITMAP_BLUE_NOISE_MATRIX_64X64,   /*!< 64x64 blue-noise threshold matrix. */
    EPD_ASSET_BITMAP_BLUE_NOISE_MATRIX_128X128, /*!< 128x128 blue-noise threshold matrix. */
    EPD_ASSET_BITMAP_BLUE_NOISE_MATRIX_256X256, /*!< 256x256 blue-noise threshold matrix. */
} epd_asset_bitmap_blue_noise_matrix_size_t;

/**
 * @brief Blue-noise dithering generation configuration.
 */
typedef struct {
    epd_gfx_format_t                           format;       /*!< Output frame format. */
    epd_asset_bitmap_blue_noise_matrix_size_t  matrix_size;  /*!< Blue-noise threshold matrix size. */
} epd_asset_bitmap_blue_noise_config_t;

/**
 * @brief Generate an EPD frame view from an RGB888 image using blue-noise dithering.
 *
 * The function allocates output buffers and stores them in `out_view`. The caller
 * owns those buffers and must release them itself.
 *
 * @param source RGB888 source image.
 * @param config Blue-noise dithering generation configuration.
 * @param out_view Pointer to receive the generated frame view.
 * @return `EPD_OK` on success, otherwise an error code from `epd_err_t`.
 */
epd_err_t epd_asset_bitmap_generate_blue_noise(const epd_asset_bitmap_source_t* source,
    const epd_asset_bitmap_blue_noise_config_t* config, epd_gfx_frame_view_t* out_view);

/**
 * @brief Generate an EPD frame view from an RGB888 image using random dithering.
 *
 * Random dithering uses a deterministic coordinate hash as the per-pixel
 * threshold source. The function allocates output buffers and stores them in
 * `out_view`. The caller owns those buffers and must release them itself.
 *
 * @param source RGB888 source image.
 * @param format Output frame format.
 * @param out_view Pointer to receive the generated frame view.
 * @return `EPD_OK` on success, otherwise an error code from `epd_err_t`.
 */
epd_err_t epd_asset_bitmap_generate_random(const epd_asset_bitmap_source_t* source,
    epd_gfx_format_t format, epd_gfx_frame_view_t* out_view);

/**
 * @brief Generate an EPD frame view from an RGB888 image using Floyd-Steinberg error diffusion.
 *
 * The function allocates output buffers and stores them in `out_view`. The caller
 * owns those buffers and must release them itself.
 *
 * @param source RGB888 source image.
 * @param format Output frame format.
 * @param out_view Pointer to receive the generated frame view.
 * @return `EPD_OK` on success, otherwise an error code from `epd_err_t`.
 */
epd_err_t epd_asset_bitmap_generate_floyd_steinberg(const epd_asset_bitmap_source_t* source,
    epd_gfx_format_t format, epd_gfx_frame_view_t* out_view);

/**
 * @brief Destroy a frame view generated by a bitmap generator.
 *
 * @param view Generated frame view whose buffers should be released.
 * @return `EPD_OK` on success, otherwise an error code from `epd_err_t`.
 */
epd_err_t epd_asset_bitmap_destroy_frame_view(epd_gfx_frame_view_t* view);

#ifdef __cplusplus
}
#endif

#endif // !_EPD_ASSET_BITMAP_GENERATOR_H_
