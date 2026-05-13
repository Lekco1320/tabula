/**
 * @file FontGlyphPreviewWidget.hpp
 * @brief Glyph metric preview widget for EGF font assets.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2026-05-13
 * @license MIT
 */

#pragma once

#ifndef _FONTGLYPHPREVIEWWIDGET_HPP_
#define _FONTGLYPHPREVIEWWIDGET_HPP_

#include <stdint.h>
#include <QImage>
#include <QWidget>

#include "common/Common.h"

LEKCO_BEGIN_NAMESPACE

class FontGlyphPreviewWidget
    : public QWidget
{
    Q_OBJECT

public:
    explicit FontGlyphPreviewWidget(QWidget* parent = nullptr);

    void setGlyph(const QImage& image, int16_t xoffset, int16_t yoffset, int16_t advance,
        int16_t ascent, int16_t lineHeight);
    void clearGlyph();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QImage  m_image;
    bool    m_hasGlyph    = false;
    int16_t m_xoffset     = 0;
    int16_t m_yoffset     = 0;
    int16_t m_advance     = 0;
    int16_t m_ascent      = 0;
    int16_t m_lineHeight  = 0;
};

LEKCO_END_NAMESPACE

#endif // !_FONTGLYPHPREVIEWWIDGET_HPP_
