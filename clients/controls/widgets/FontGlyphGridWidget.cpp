/**
 * @file FontGlyphGridWidget.cpp
 * @brief Lazy glyph grid widget implementation for EGF font assets.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2026-05-13
 * @license MIT
 */

#include <QColor>
#include <QFont>
#include <QFrame>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>
#include <QSet>
#include <QSize>
#include <QtGlobal>
#include <epd_gfx/glyph.h>

#include "controls/widgets/FontGlyphGridWidget.hpp"
#include "controls/widgets/FontGlyphImage.hpp"

LEKCO_BEGIN_NAMESPACE

BEGIN_NAMESPACE()

constexpr int kHeaderHeight   = 36;
constexpr int kCellSize       = 72;
constexpr int kCellGap        = 8;
constexpr int kGlyphPadding   = 8;
constexpr int kCodepointSpace = 22;
constexpr int kCachePadding   = 2;
constexpr int kHeaderMargin   = 12;
constexpr int kDisclosureSize = 8;

END_NAMESPACE

FontGlyphGridWidget::FontGlyphGridWidget(QWidget* parent)
    : QAbstractScrollArea(parent)
{
    setFrameShape(QFrame::NoFrame);
    setMouseTracking(true);
}

void FontGlyphGridWidget::setFontAsset(epd_gfx_font_asset_t asset)
{
    m_asset = asset;
    m_cache.clear();
    viewport()->update();
}

void FontGlyphGridWidget::setSections(const QVector<Section>& sections)
{
    m_sections = sections;
    m_cache.clear();
    updateScrollRange();
    viewport()->update();
}

void FontGlyphGridWidget::clear()
{
    m_asset             = nullptr;
    m_selectedSize      = 0U;
    m_selectedCodepoint = 0U;
    m_sections.clear();
    m_cache.clear();
    updateScrollRange();
    viewport()->update();
}

void FontGlyphGridWidget::selectGlyph(uint16_t size, uint32_t codepoint)
{
    m_selectedSize      = size;
    m_selectedCodepoint = codepoint;
    viewport()->update();
}

void FontGlyphGridWidget::clearSelection()
{
    m_selectedSize      = 0U;
    m_selectedCodepoint = 0U;
    viewport()->update();
}

void FontGlyphGridWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)

    QPainter painter(viewport());
    painter.fillRect(viewport()->rect(), palette().window());

    const int scrollY = verticalScrollBar()->value();
    const int viewH   = viewport()->height();
    int y             = -scrollY;
    QSet<quint64> visibleKeys;

    for (const Section& section : m_sections) {
        const QRect header = headerRect(y);
        if (header.bottom() >= -kHeaderHeight && header.top() <= viewH + kHeaderHeight) {
            const QPoint center(kHeaderMargin + kDisclosureSize / 2, header.center().y());
            QPolygon triangle;
            if (section.expanded) {
                triangle << QPoint(center.x() - kDisclosureSize / 2, center.y() - kDisclosureSize / 4)
                         << QPoint(center.x() + kDisclosureSize / 2, center.y() - kDisclosureSize / 4)
                         << QPoint(center.x(), center.y() + kDisclosureSize / 2);
            } else {
                triangle << QPoint(center.x() - kDisclosureSize / 4, center.y() - kDisclosureSize / 2)
                         << QPoint(center.x() - kDisclosureSize / 4, center.y() + kDisclosureSize / 2)
                         << QPoint(center.x() + kDisclosureSize / 2, center.y());
            }

            painter.setRenderHint(QPainter::Antialiasing, true);
            painter.setPen(Qt::NoPen);
            painter.setBrush(palette().text().color());
            painter.drawPolygon(triangle);
            painter.setRenderHint(QPainter::Antialiasing, false);

            QFont     titleFont     = painter.font();
            const int basePointSize = titleFont.pointSize();
            const int basePixelSize = titleFont.pixelSize();
            titleFont.setBold(true);
            if (basePointSize > 0) {
                titleFont.setPointSize(basePointSize + 2);
            } else if (basePixelSize > 0) {
                titleFont.setPixelSize(basePixelSize + 2);
            }
            painter.setFont(titleFont);
            painter.setPen(palette().text().color());
            const QRect   titleRect = header.adjusted(kHeaderMargin + kDisclosureSize + 8, 0, -kHeaderMargin, 0);
            const QString title     = QStringLiteral("Size %1").arg(section.size);
            painter.drawText(titleRect, Qt::AlignVCenter | Qt::AlignLeft, title);

            const int titleWidth = painter.fontMetrics().horizontalAdvance(title);
            QFont     countFont  = titleFont;
            countFont.setBold(false);
            if (basePointSize > 0) {
                countFont.setPointSize(basePointSize);
            } else if (basePixelSize > 0) {
                countFont.setPixelSize(basePixelSize);
            }
            painter.setFont(countFont);
            painter.setPen(QColor(90, 90, 90));
            painter.drawText(titleRect.adjusted(titleWidth + 10, 0, 0, 0), Qt::AlignVCenter | Qt::AlignLeft,
                QStringLiteral("%1 glyphs").arg(section.codepoints.size()));
        }
        y += kHeaderHeight;

        if (!section.expanded) {
            continue;
        }

        const int columns = columnCount();
        const int rows    = columns > 0 ? (section.codepoints.size() + columns - 1) / columns : 0;
        for (int row = 0; row < rows; ++row) {
            const int rowY = y + row * (kCellSize + kCellGap);
            if (rowY + kCellSize < -kCellSize * kCachePadding
                || rowY > viewH + kCellSize * kCachePadding) {
                continue;
            }

            for (int col = 0; col < columns; ++col) {
                const int index = row * columns + col;
                if (index >= section.codepoints.size()) {
                    break;
                }

                const uint32_t codepoint = section.codepoints[index];
                const QRect   cell       = cellRect(y, index);
                const bool    visible    = cell.bottom() >= 0 && cell.top() <= viewH;
                visibleKeys.insert(cacheKey(section.size, codepoint));

                if (!visible) {
                    continue;
                }

                const bool selected = m_selectedSize == section.size && m_selectedCodepoint == codepoint;
                painter.setPen(selected ? QColor(40, 110, 220) : QColor(180, 180, 180));
                painter.setBrush(selected ? QColor(220, 235, 255) : QColor(255, 255, 255));
                painter.drawRoundedRect(cell.adjusted(1, 1, -1, -1), 5, 5);

                const GlyphRenderData glyph = glyphRenderData(section.size, codepoint);
                if (glyph.valid) {
                    const int   scale      = sectionScale(section);
                    const QRect lineRect   = cell.adjusted(kGlyphPadding, kGlyphPadding,
                        -kGlyphPadding, -kGlyphPadding - kCodepointSpace);
                    const int   lineHeight = qMax(1, static_cast<int>(section.lineHeight)) * scale;
                    const int   lineTop    = lineRect.top() + qMax(0, (lineRect.height() - lineHeight) / 2);
                    const int   baseline   = lineTop + static_cast<int>(section.ascent) * scale;
                    const int   advance    = qMax(1, static_cast<int>(glyph.advance)) * scale;
                    const int   originX    = lineRect.center().x() - advance / 2;
                    const QPoint topLeft(originX + static_cast<int>(glyph.xoffset) * scale,
                        baseline + static_cast<int>(glyph.yoffset) * scale);

                    if (!glyph.image.isNull()) {
                        painter.save();
                        painter.setClipRect(lineRect);
                        const QSize targetSize(glyph.image.width() * scale, glyph.image.height() * scale);
                        painter.drawImage(QRect(topLeft, targetSize), glyph.image);
                        painter.restore();
                    }
                }

                painter.setPen(QColor(80, 80, 80));
                painter.drawText(cell.adjusted(4, 0, -4, -4), Qt::AlignBottom | Qt::AlignHCenter,
                    formatCodepoint(codepoint));
            }
        }
        y += rows * (kCellSize + kCellGap);
    }

    pruneCache(visibleKeys);
}

void FontGlyphGridWidget::mousePressEvent(QMouseEvent* event)
{
    const int scrollY = verticalScrollBar()->value();
    int y             = -scrollY;

    for (Section& section : m_sections) {
        const QRect header = headerRect(y);
        if (header.contains(event->pos())) {
            section.expanded = !section.expanded;
            updateScrollRange();
            viewport()->update();
            return;
        }
        y += kHeaderHeight;

        if (!section.expanded) {
            continue;
        }

        const int columns = columnCount();
        const int rows    = columns > 0 ? (section.codepoints.size() + columns - 1) / columns : 0;
        const int localY  = event->pos().y() - y;
        if (localY >= 0 && localY < rows * (kCellSize + kCellGap)) {
            const int row = localY / (kCellSize + kCellGap);
            const int col = (event->pos().x() - kCellGap) / (kCellSize + kCellGap);
            if (col >= 0 && col < columns) {
                const int index = row * columns + col;
                if (index < section.codepoints.size() && cellRect(y, index).contains(event->pos())) {
                    selectGlyph(section.size, section.codepoints[index]);
                    emit glyphSelected(section.size, section.codepoints[index]);
                    return;
                }
            }
        }
        y += rows * (kCellSize + kCellGap);
    }

    clearSelection();
    emit selectionCleared();
}

void FontGlyphGridWidget::resizeEvent(QResizeEvent* event)
{
    QAbstractScrollArea::resizeEvent(event);
    updateScrollRange();
}

int FontGlyphGridWidget::columnCount() const
{
    return qMax(1, (viewport()->width() + kCellGap) / (kCellSize + kCellGap));
}

int FontGlyphGridWidget::sectionHeight(const Section& section) const
{
    if (!section.expanded) {
        return kHeaderHeight;
    }

    const int columns = columnCount();
    const int rows    = columns > 0 ? (section.codepoints.size() + columns - 1) / columns : 0;
    return kHeaderHeight + rows * (kCellSize + kCellGap);
}

int FontGlyphGridWidget::contentHeight() const
{
    int height = 0;
    for (const Section& section : m_sections) {
        height += sectionHeight(section);
    }
    return height;
}

int FontGlyphGridWidget::sectionScale(const Section& section) const
{
    const int lineHeight      = qMax(1, static_cast<int>(section.lineHeight));
    const int availableHeight = kCellSize - 2 * kGlyphPadding - kCodepointSpace;
    return qMax(1, availableHeight / lineHeight);
}

QRect FontGlyphGridWidget::headerRect(int y) const
{
    return QRect(0, y, viewport()->width(), kHeaderHeight);
}

QRect FontGlyphGridWidget::cellRect(int y, int index) const
{
    const int columns = columnCount();
    const int row     = index / columns;
    const int col     = index % columns;
    return QRect(col * (kCellSize + kCellGap) + kCellGap, y + row * (kCellSize + kCellGap),
        kCellSize, kCellSize);
}

FontGlyphGridWidget::GlyphRenderData FontGlyphGridWidget::glyphRenderData(uint16_t size, uint32_t codepoint)
{
    const quint64 key = cacheKey(size, codepoint);
    auto          it  = m_cache.find(key);
    if (it != m_cache.end()) {
        return it.value();
    }
    if (!m_asset) {
        return GlyphRenderData();
    }

    epd_gfx_font_asset_glyph_key_t glyphKey;
    glyphKey.codepoint = codepoint;
    glyphKey.size      = size;

    epd_gfx_glyph_t glyph = nullptr;
    if (epd_gfx_font_asset_get_glyph(m_asset, glyphKey, &glyph) != EPD_OK || !glyph) {
        return GlyphRenderData();
    }

    GlyphRenderData renderData;
    renderData.valid   = true;
    renderData.xoffset = epd_gfx_glyph_get_xoffset(glyph);
    renderData.yoffset = epd_gfx_glyph_get_yoffset(glyph);
    renderData.advance = epd_gfx_glyph_get_advance(glyph);
    renderData.image   = toMonoImage(glyph);
    epd_gfx_glyph_destroy(glyph);
    m_cache.insert(key, renderData);
    return renderData;
}

void FontGlyphGridWidget::updateScrollRange()
{
    const int max = qMax(0, contentHeight() - viewport()->height());
    verticalScrollBar()->setRange(0, max);
    verticalScrollBar()->setPageStep(viewport()->height());
}

void FontGlyphGridWidget::pruneCache(const QSet<quint64>& visibleKeys)
{
    for (auto it = m_cache.begin(); it != m_cache.end();) {
        if (!visibleKeys.contains(it.key())) {
            it = m_cache.erase(it);
        } else {
            ++it;
        }
    }
}

QString FontGlyphGridWidget::formatCodepoint(uint32_t codepoint)
{
    return QStringLiteral("U+%1").arg(codepoint, codepoint <= 0xFFFFU ? 4 : 6, 16, QLatin1Char('0')).toUpper();
}

quint64 FontGlyphGridWidget::cacheKey(uint16_t size, uint32_t codepoint)
{
    return (static_cast<quint64>(size) << 32U) | static_cast<quint64>(codepoint);
}

LEKCO_END_NAMESPACE
