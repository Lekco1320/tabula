/**
 * @file FontGlyphPreviewWidget.cpp
 * @brief Glyph metric preview widget implementation for EGF font assets.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2026-05-13
 * @license MIT
 */

#include <QColor>
#include <QPainter>
#include <QPaintEvent>
#include <QPoint>
#include <QRect>
#include <QtGlobal>

#include "controls/widgets/FontGlyphPreviewWidget.hpp"

LEKCO_BEGIN_NAMESPACE

BEGIN_NAMESPACE()

constexpr int kPreviewMargin = 12;

END_NAMESPACE

FontGlyphPreviewWidget::FontGlyphPreviewWidget(QWidget* parent)
    : QWidget(parent)
{
    setFixedSize(160, 160);
}

void FontGlyphPreviewWidget::setGlyph(const QImage& image, int16_t xoffset, int16_t yoffset, int16_t advance,
    int16_t ascent, int16_t lineHeight)
{
    m_image      = image;
    m_hasGlyph   = true;
    m_xoffset    = xoffset;
    m_yoffset    = yoffset;
    m_advance    = advance;
    m_ascent     = ascent;
    m_lineHeight = lineHeight;
    update();
}

void FontGlyphPreviewWidget::clearGlyph()
{
    m_image      = QImage();
    m_hasGlyph   = false;
    m_xoffset    = 0;
    m_yoffset    = 0;
    m_advance    = 0;
    m_ascent     = 0;
    m_lineHeight = 0;
    update();
}

void FontGlyphPreviewWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.fillRect(rect(), Qt::white);

    if (!m_hasGlyph || m_lineHeight <= 0) {
        return;
    }

    const QRect content    = rect().adjusted(kPreviewMargin, kPreviewMargin, -kPreviewMargin, -kPreviewMargin);
    const int   advance    = qMax(1, static_cast<int>(m_advance));
    const int   lineHeight = qMax(1, static_cast<int>(m_lineHeight));
    const int   lineLeft   = content.center().x() - advance / 2;
    const int   lineTop    = content.center().y() - lineHeight / 2;
    const int   baseline   = lineTop + static_cast<int>(m_ascent);
    const int   glyphLeft  = lineLeft + static_cast<int>(m_xoffset);
    const int   glyphTop   = baseline + static_cast<int>(m_yoffset);
    const QRect glyphRect(QPoint(glyphLeft, glyphTop), m_image.size());

    painter.save();
    painter.setClipRect(rect());
    painter.setPen(QColor(0, 200, 255));
    painter.drawLine(rect().left(), baseline, rect().right(), baseline);

    if (!m_image.isNull()) {
        painter.drawImage(glyphRect.topLeft(), m_image);
    }
    painter.restore();
}

LEKCO_END_NAMESPACE
