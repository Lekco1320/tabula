/**
 * @file FontGlyphImage.cpp
 * @brief EGF glyph image conversion helper implementation.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2026-05-13
 * @license MIT
 */

#include <cstring>
#include <QImage>
#include <epd_gfx/glyph.h>

#include "controls/widgets/FontGlyphImage.hpp"

LEKCO_BEGIN_NAMESPACE

QImage toMonoImage(epd_gfx_glyph_t glyph)
{
    const uint16_t width  = epd_gfx_glyph_get_width(glyph);
    const uint16_t height = epd_gfx_glyph_get_height(glyph);
    const uint8_t* data   = epd_gfx_glyph_get_data(glyph);
    if (width == 0U || height == 0U || !data) {
        return QImage();
    }

    const uint32_t stride = (width + 7U) / 8U;
    QImage image(width, height, QImage::Format_Mono);
    image.setColor(0, qRgba(0, 0, 0, 0));
    image.setColor(1, qRgb(0, 0, 0));
    image.fill(0);
    for (uint16_t row = 0; row < height; ++row) {
        std::memcpy(image.scanLine(row), data + row * stride, stride);
    }

    return image;
}

LEKCO_END_NAMESPACE
