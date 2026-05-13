/**
 * @file FontGlyphImage.hpp
 * @brief EGF glyph image conversion helpers.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2026-05-13
 * @license MIT
 */

#pragma once

#ifndef _FONTGLYPHIMAGE_HPP_
#define _FONTGLYPHIMAGE_HPP_

#include <QImage>
#include <epd_gfx/glyph.h>

#include "common/Common.h"

LEKCO_BEGIN_NAMESPACE

QImage toMonoImage(epd_gfx_glyph_t glyph);

LEKCO_END_NAMESPACE

#endif // !_FONTGLYPHIMAGE_HPP_
