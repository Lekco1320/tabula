/**
 * @file bitmap_generator.c
 * @brief Bitmap generator API implementation.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2026-05-18
 * @license MIT
 */

#include <stdlib.h>
#include <string.h>

#include <epd_core/math.h>
#include <epd_gfx/codec.h>

#include "epd_asset/bitmap_generator.h"
#include "epd_asset/bitmap_dither_matrix.h"

static float epd_asset_bitmap_clamp(float x, float l, float r)
{
    return EPD_MAX(l, EPD_MIN(x, r));
}

static uint8_t epd_asset_bitmap_pixel_rgb_luma(uint8_t red, uint8_t green, uint8_t blue)
{
    return (uint8_t)((77U * red + 150U * green + 29U * blue) >> 8U);
}

static uint8_t epd_asset_bitmap_pixel_saturation(uint8_t red, uint8_t green, uint8_t blue)
{
    uint8_t max = EPD_MAX(red, EPD_MAX(green, blue));
    uint8_t min = EPD_MIN(red, EPD_MIN(green, blue));
    return (uint8_t)(max - min);
}

static uint16_t epd_asset_bitmap_output_stride(uint16_t width, epd_gfx_format_t format)
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

static epd_err_t epd_asset_bitmap_validate_source(const epd_asset_bitmap_source_t* source)
{
    if (!source || !source->data) {
        return EPD_ERR_INVALID_ARG;
    }
    if (source->width == 0U || source->height == 0U) {
        return EPD_ERR_INVALID_SIZE;
    }
    if (source->stride < (uint32_t)source->width * 3U) {
        return EPD_ERR_INVALID_SIZE;
    }

    return EPD_OK;
}

static epd_err_t epd_asset_bitmap_alloc_frame(uint16_t width, uint16_t height,
    epd_gfx_format_t format, epd_gfx_frame_view_t* out_view)
{
    uint16_t stride = epd_asset_bitmap_output_stride(width, format);
    if (stride == 0U) {
        return EPD_ERR_INVALID_ARG;
    }

    uint32_t bytes = (uint32_t)stride * height;
    memset(out_view, 0, sizeof(*out_view));
    out_view->format = format;
    out_view->width  = width;
    out_view->height = height;
    out_view->stride = stride;

    switch (format) {
    case EPD_GFX_FORMAT_NATIVE: {
        uint8_t* buf_native = (uint8_t*)malloc(bytes);
        if (!buf_native) {
            return EPD_ERR_NO_MEM;
        }
        epd_gfx_native_set_bytes(buf_native, bytes, EPD_GFX_WHITE);
        out_view->buf_native = buf_native;
        return EPD_OK;
    }

    case EPD_GFX_FORMAT_PLANES: {
        uint8_t* buf_wht = (uint8_t*)malloc(bytes);
        uint8_t* buf_red = (uint8_t*)malloc(bytes);
        if (!buf_wht || !buf_red) {
            free(buf_wht);
            free(buf_red);
            memset(out_view, 0, sizeof(*out_view));
            return EPD_ERR_NO_MEM;
        }
        epd_gfx_planes_set_bytes(buf_wht, buf_red, bytes, EPD_GFX_WHITE);
        out_view->buf_wht = buf_wht;
        out_view->buf_red = buf_red;
        return EPD_OK;
    }

    default:
        memset(out_view, 0, sizeof(*out_view));
        return EPD_ERR_INVALID_ARG;
    }
}

static void epd_asset_bitmap_set_pixel(epd_gfx_frame_view_t* view,
    uint16_t x, uint16_t y, epd_gfx_color_t color)
{
    switch (view->format) {
    case EPD_GFX_FORMAT_NATIVE: {
        uint8_t* row = (uint8_t*)view->buf_native + (uint32_t)y * view->stride;
        epd_gfx_native_set_pixel(&row[x / 2U], x, color);
        break;
    }

    case EPD_GFX_FORMAT_PLANES: {
        uint8_t* row_wht = (uint8_t*)view->buf_wht + (uint32_t)y * view->stride;
        uint8_t* row_red = (uint8_t*)view->buf_red + (uint32_t)y * view->stride;
        epd_gfx_planes_set_pixel(&row_wht[x / 8U], &row_red[x / 8U], x, color);
        break;
    }

    default:
        break;
    }
}

static epd_gfx_color_t epd_asset_bitmap_threshold_color(uint8_t red, uint8_t green,
    uint8_t blue, const epd_asset_bitmap_threshold_config_t* config)
{
    uint8_t luma       = epd_asset_bitmap_pixel_rgb_luma(red, green, blue);
    uint8_t saturation = epd_asset_bitmap_pixel_saturation(red, green, blue);
    int16_t red_score  = (int16_t)red - (int16_t)EPD_MAX(green, blue);

    if (red_score >= config->red_threshold && saturation >= config->red_saturation) {
        return EPD_GFX_RED;
    }
    if (luma < config->black_threshold) {
        return EPD_GFX_BLACK;
    }
    return EPD_GFX_WHITE;
}

epd_err_t epd_asset_bitmap_generate_threshold(const epd_asset_bitmap_source_t* source,
    const epd_asset_bitmap_threshold_config_t* config, epd_gfx_frame_view_t* out_view)
{
    if (!config || !out_view) {
        return EPD_ERR_INVALID_ARG;
    }

    epd_err_t ret = epd_asset_bitmap_validate_source(source);
    if (ret != EPD_OK) {
        return ret;
    }
    if (config->format != EPD_GFX_FORMAT_NATIVE && config->format != EPD_GFX_FORMAT_PLANES) {
        return EPD_ERR_INVALID_ARG;
    }

    ret = epd_asset_bitmap_alloc_frame(source->width, source->height, config->format, out_view);
    if (ret != EPD_OK) {
        return ret;
    }

    for (uint16_t y = 0U; y < source->height; ++y) {
        const uint8_t* row = source->data + (uint32_t)y * source->stride;
        for (uint16_t x = 0U; x < source->width; ++x) {
            const uint8_t*  pixel = row + (uint32_t)x * 3U;
            epd_gfx_color_t color = epd_asset_bitmap_threshold_color(pixel[0], pixel[1],
                pixel[2], config);
            epd_asset_bitmap_set_pixel(out_view, x, y, color);
        }
    }

    return EPD_OK;
}

static epd_gfx_color_t epd_asset_bitmap_dither_color(uint8_t red, uint8_t green,
    uint8_t blue, float threshold)
{
    float r = (float)red / 255.0f;
    float g = (float)green / 255.0f;
    float b = (float)blue / 255.0f;
    float u = (g + b) / 2.0f;

    float rk = epd_asset_bitmap_clamp(r - u, 0.0f, 1.0f);
    float wk = epd_asset_bitmap_clamp(u, 0.0f, 1.0f - rk);
    float bk = 1.0f - rk - wk;

    if (threshold < bk) {
        return EPD_GFX_BLACK;
    } else if (threshold < bk + wk) {
        return EPD_GFX_WHITE;
    }
    return EPD_GFX_RED;
}

static epd_err_t epd_asset_bitmap_generate_matrix_dither(
    const epd_asset_bitmap_source_t* source, epd_gfx_format_t format,
    uint16_t matrix_size, uint16_t threshold_count, const uint8_t* matrix,
    epd_gfx_frame_view_t* out_view)
{
    epd_err_t ret = epd_asset_bitmap_alloc_frame(source->width, source->height, format, out_view);
    if (ret != EPD_OK) {
        return ret;
    }

    for (uint16_t y = 0U; y < source->height; ++y) {
        const uint8_t* row = source->data + (uint32_t)y * source->stride;
        for (uint16_t x = 0U; x < source->width; ++x) {
            const uint8_t* pixel = row + (uint32_t)x * 3U;
            uint32_t       index = (uint32_t)(y % matrix_size) * matrix_size +
                (x % matrix_size);
            float threshold = ((float)matrix[index] + 0.5f) /
                (float)threshold_count;
            epd_gfx_color_t color = epd_asset_bitmap_dither_color(pixel[0], pixel[1],
                pixel[2], threshold);
            epd_asset_bitmap_set_pixel(out_view, x, y, color);
        }
    }

    return EPD_OK;
}

epd_err_t epd_asset_bitmap_generate_ordered(const epd_asset_bitmap_source_t* source,
    const epd_asset_bitmap_ordered_config_t* config, epd_gfx_frame_view_t* out_view)
{
    if (!config || !out_view) {
        return EPD_ERR_INVALID_ARG;
    }

    epd_err_t ret = epd_asset_bitmap_validate_source(source);
    if (ret != EPD_OK) {
        return ret;
    }
    if (config->format != EPD_GFX_FORMAT_NATIVE && config->format != EPD_GFX_FORMAT_PLANES) {
        return EPD_ERR_INVALID_ARG;
    }

    uint8_t        n      = 0U;
    const uint8_t* matrix = NULL;
    switch (config->matrix_size) {
    case EPD_ASSET_BITMAP_ORDERED_MATRIX_2X2:
        n      = 2;
        matrix = &EPD_ASSET_BITMAP_DITHER_MATRIX_ORDERED_2X2[0][0];
        break;

    case EPD_ASSET_BITMAP_ORDERED_MATRIX_4X4:
        n      = 4;
        matrix = &EPD_ASSET_BITMAP_DITHER_MATRIX_ORDERED_4X4[0][0];
        break;

    case EPD_ASSET_BITMAP_ORDERED_MATRIX_8X8:
        n      = 8;
        matrix = &EPD_ASSET_BITMAP_DITHER_MATRIX_ORDERED_8X8[0][0];
        break;

    default:
        return EPD_ERR_INVALID_ARG;
    }

    return epd_asset_bitmap_generate_matrix_dither(source, config->format, n,
        (uint16_t)(n * n), matrix, out_view);
}

epd_err_t epd_asset_bitmap_generate_blue_noise(const epd_asset_bitmap_source_t* source,
    const epd_asset_bitmap_blue_noise_config_t* config, epd_gfx_frame_view_t* out_view)
{
    if (!config || !out_view) {
        return EPD_ERR_INVALID_ARG;
    }

    epd_err_t ret = epd_asset_bitmap_validate_source(source);
    if (ret != EPD_OK) {
        return ret;
    }
    if (config->format != EPD_GFX_FORMAT_NATIVE && config->format != EPD_GFX_FORMAT_PLANES) {
        return EPD_ERR_INVALID_ARG;
    }

    uint16_t       n      = 0U;
    const uint8_t* matrix = NULL;
    switch (config->matrix_size) {
    case EPD_ASSET_BITMAP_BLUE_NOISE_MATRIX_64X64:
        n      = EPD_ASSET_BITMAP_DITHER_MATRIX_BLUE_NOISE_64_SIZE;
        matrix = &EPD_ASSET_BITMAP_DITHER_MATRIX_BLUE_NOISE_64X64[0][0];
        break;

    case EPD_ASSET_BITMAP_BLUE_NOISE_MATRIX_128X128:
        n      = EPD_ASSET_BITMAP_DITHER_MATRIX_BLUE_NOISE_128_SIZE;
        matrix = &EPD_ASSET_BITMAP_DITHER_MATRIX_BLUE_NOISE_128X128[0][0];
        break;

    case EPD_ASSET_BITMAP_BLUE_NOISE_MATRIX_256X256:
        n      = EPD_ASSET_BITMAP_DITHER_MATRIX_BLUE_NOISE_256_SIZE;
        matrix = &EPD_ASSET_BITMAP_DITHER_MATRIX_BLUE_NOISE_256X256[0][0];
        break;

    default:
        return EPD_ERR_INVALID_ARG;
    }

    return epd_asset_bitmap_generate_matrix_dither(source, config->format, n, 256U,
        matrix, out_view);
}

static uint32_t epd_asset_bitmap_hash_two_uint16(uint16_t key1, uint16_t key2)
{
    uint32_t combined = ((uint32_t)key1 << 16) | key2;
    combined ^= combined >> 16;
    combined *= 0x85ebca6b;
    combined ^= combined >> 13;
    combined *= 0xc2b2ae35;
    combined ^= combined >> 16;
    return combined;
}

epd_err_t epd_asset_bitmap_generate_random(const epd_asset_bitmap_source_t* source,
    epd_gfx_format_t format, epd_gfx_frame_view_t* out_view)
{
    if (!out_view) {
        return EPD_ERR_INVALID_ARG;
    }

    epd_err_t ret = epd_asset_bitmap_validate_source(source);
    if (ret != EPD_OK) {
        return ret;
    }
    if (format != EPD_GFX_FORMAT_NATIVE && format != EPD_GFX_FORMAT_PLANES) {
        return EPD_ERR_INVALID_ARG;
    }

    ret = epd_asset_bitmap_alloc_frame(source->width, source->height, format, out_view);
    if (ret != EPD_OK) {
        return ret;
    }

    for (uint16_t y = 0U; y < source->height; ++y) {
        const uint8_t* row = source->data + (uint32_t)y * source->stride;
        for (uint16_t x = 0U; x < source->width; ++x) {
            const uint8_t*  pixel     = row + (uint32_t)x * 3U;
            float           threshold = (float)epd_asset_bitmap_hash_two_uint16(x, y) /
                                        4294967296.0f;
            epd_gfx_color_t color     = epd_asset_bitmap_dither_color(pixel[0], pixel[1],
                pixel[2], threshold);
            epd_asset_bitmap_set_pixel(out_view, x, y, color);
        }
    }

    return EPD_OK;
}

typedef struct {
    float red;
    float green;
    float blue;
} epd_asset_bitmap_rgb_error_t;

static float epd_asset_bitmap_rgb_distance(float r0, float g0, float b0,
    float r1, float g1, float b1)
{
    float dr = r0 - r1;
    float dg = g0 - g1;
    float db = b0 - b1;

    return dr * dr + dg * dg + db * db;
}

static epd_gfx_color_t epd_asset_bitmap_nearest_color(float red, float green,
    float blue)
{
    float min_distance = epd_asset_bitmap_rgb_distance(red, green, blue,
        0.0f, 0.0f, 0.0f);
    epd_gfx_color_t color = EPD_GFX_BLACK;

    float distance = epd_asset_bitmap_rgb_distance(red, green, blue, 255.0f, 255.0f,
        255.0f);
    if (distance < min_distance) {
        min_distance = distance;
        color        = EPD_GFX_WHITE;
    }

    distance = epd_asset_bitmap_rgb_distance(red, green, blue, 255.0f, 0.0f, 0.0f);
    if (distance < min_distance) {
        color = EPD_GFX_RED;
    }

    return color;
}

static void epd_asset_bitmap_color_rgb(epd_gfx_color_t color, uint8_t* red,
    uint8_t* green, uint8_t* blue)
{
    switch (color) {
    case EPD_GFX_WHITE:
        *red   = 255U;
        *green = 255U;
        *blue  = 255U;
        break;

    case EPD_GFX_RED:
        *red   = 255U;
        *green = 0U;
        *blue  = 0U;
        break;

    case EPD_GFX_BLACK:
    default:
        *red   = 0U;
        *green = 0U;
        *blue  = 0U;
        break;
    }
}

static void epd_asset_bitmap_add_error(epd_asset_bitmap_rgb_error_t* row,
    int32_t width, int32_t x, const epd_asset_bitmap_rgb_error_t* error, float weight)
{
    if (x < 0 || x >= width) {
        return;
    }

    row[x].red   += error->red * weight;
    row[x].green += error->green * weight;
    row[x].blue  += error->blue * weight;
}

static void epd_asset_bitmap_diffuse_error(epd_asset_bitmap_rgb_error_t* current_errors,
    epd_asset_bitmap_rgb_error_t* next_errors, int32_t width, int32_t x,
    const epd_asset_bitmap_rgb_error_t* error, bool left_to_right)
{
    if (left_to_right) {
        epd_asset_bitmap_add_error(current_errors, width, x + 1, error, 7.0f / 16.0f);
        epd_asset_bitmap_add_error(next_errors, width, x - 1, error, 3.0f / 16.0f);
        epd_asset_bitmap_add_error(next_errors, width, x,     error, 5.0f / 16.0f);
        epd_asset_bitmap_add_error(next_errors, width, x + 1, error, 1.0f / 16.0f);
    } else {
        epd_asset_bitmap_add_error(current_errors, width, x - 1, error, 7.0f / 16.0f);
        epd_asset_bitmap_add_error(next_errors, width, x + 1, error, 3.0f / 16.0f);
        epd_asset_bitmap_add_error(next_errors, width, x,     error, 5.0f / 16.0f);
        epd_asset_bitmap_add_error(next_errors, width, x - 1, error, 1.0f / 16.0f);
    }
}

epd_err_t epd_asset_bitmap_generate_floyd_steinberg(const epd_asset_bitmap_source_t* source,
    epd_gfx_format_t format, epd_gfx_frame_view_t* out_view)
{
    if (!out_view) {
        return EPD_ERR_INVALID_ARG;
    }

    epd_err_t ret = epd_asset_bitmap_validate_source(source);
    if (ret != EPD_OK) {
        return ret;
    }
    if (format != EPD_GFX_FORMAT_NATIVE && format != EPD_GFX_FORMAT_PLANES) {
        return EPD_ERR_INVALID_ARG;
    }

    size_t row_bytes = (size_t)source->width * sizeof(epd_asset_bitmap_rgb_error_t);
    epd_asset_bitmap_rgb_error_t* current_errors =
        (epd_asset_bitmap_rgb_error_t*)calloc(source->width, sizeof(*current_errors));
    epd_asset_bitmap_rgb_error_t* next_errors =
        (epd_asset_bitmap_rgb_error_t*)calloc(source->width, sizeof(*next_errors));
    if (!current_errors || !next_errors) {
        free(current_errors);
        free(next_errors);
        return EPD_ERR_NO_MEM;
    }

    ret = epd_asset_bitmap_alloc_frame(source->width, source->height, format, out_view);
    if (ret != EPD_OK) {
        free(current_errors);
        free(next_errors);
        return ret;
    }

    for (uint16_t y = 0U; y < source->height; ++y) {
        const uint8_t* row           = source->data + (uint32_t)y * source->stride;
        bool           left_to_right = (y % 2U) == 0U;
        int32_t        start         = left_to_right ? 0 : (int32_t)source->width - 1;
        int32_t        end           = left_to_right ? (int32_t)source->width : -1;
        int32_t        step          = left_to_right ? 1 : -1;

        for (int32_t x = start; x != end; x += step) {
            const uint8_t* pixel = row + (uint32_t)x * 3U;
            float red = epd_asset_bitmap_clamp((float)pixel[0] + current_errors[x].red,
                0.0f, 255.0f);
            float green = epd_asset_bitmap_clamp((float)pixel[1] +
                current_errors[x].green, 0.0f, 255.0f);
            float blue = epd_asset_bitmap_clamp((float)pixel[2] + current_errors[x].blue,
                0.0f, 255.0f);

            epd_gfx_color_t color = epd_asset_bitmap_nearest_color(red, green, blue);
            epd_asset_bitmap_set_pixel(out_view, (uint16_t)x, y, color);

            uint8_t pal_red   = 0U;
            uint8_t pal_green = 0U;
            uint8_t pal_blue  = 0U;
            epd_asset_bitmap_color_rgb(color, &pal_red, &pal_green, &pal_blue);

            epd_asset_bitmap_rgb_error_t error = {
                .red   = red - (float)pal_red,
                .green = green - (float)pal_green,
                .blue  = blue - (float)pal_blue,
            };
            epd_asset_bitmap_diffuse_error(current_errors, next_errors,
                source->width, x, &error, left_to_right);
        }

        epd_asset_bitmap_rgb_error_t* tmp = current_errors;
        current_errors = next_errors;
        next_errors    = tmp;
        memset(next_errors, 0, row_bytes);
    }

    free(current_errors);
    free(next_errors);
    return EPD_OK;
}

epd_err_t epd_asset_bitmap_destroy_frame_view(epd_gfx_frame_view_t* view)
{
    if (!view) {
        return EPD_ERR_INVALID_ARG;
    }

    switch (view->format) {
    case EPD_GFX_FORMAT_NATIVE:
        free((void*)view->buf_native);
        break;

    case EPD_GFX_FORMAT_PLANES:
        free((void*)view->buf_wht);
        free((void*)view->buf_red);
        break;

    default:
        memset(view, 0, sizeof(*view));
        return EPD_ERR_INVALID_ARG;
    }

    memset(view, 0, sizeof(*view));
    return EPD_OK;
}
