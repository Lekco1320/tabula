/**
 * @file FontGlyphGridWidget.hpp
 * @brief Lazy glyph grid widget for EGF font assets.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2026-05-13
 * @license MIT
 */

#pragma once

#ifndef _FONTGLYPHGRIDWIDGET_HPP_
#define _FONTGLYPHGRIDWIDGET_HPP_

#include <stdint.h>
#include <QAbstractScrollArea>
#include <QHash>
#include <QImage>
#include <QRect>
#include <QSet>
#include <QString>
#include <QVector>
#include <epd_asset/font_asset.h>

#include "common/Common.h"

LEKCO_BEGIN_NAMESPACE

class FontGlyphGridWidget
    : public QAbstractScrollArea
{
    Q_OBJECT

public:
    struct Section {
        uint16_t          size       = 0U;
        int16_t           ascent     = 0;
        int16_t           descent    = 0;
        int16_t           lineHeight = 0;
        QVector<uint32_t> codepoints;
        bool              expanded   = true;
    };

    explicit FontGlyphGridWidget(QWidget* parent = nullptr);

    void setFontAsset(epd_asset_font_asset_t asset);
    void setSections(const QVector<Section>& sections);
    void clear();
    void selectGlyph(uint16_t size, uint32_t codepoint);
    void clearSelection();

signals:
    void glyphSelected(uint16_t size, uint32_t codepoint);
    void selectionCleared();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    struct GlyphRenderData {
        QImage  image;
        bool    valid   = false;
        int16_t xoffset = 0;
        int16_t yoffset = 0;
        int16_t advance = 0;
    };

    int columnCount(const Section& section) const;
    int sectionHeight(const Section& section) const;
    int contentHeight() const;
    int sectionScale(const Section& section) const;
    int sectionCellSize(const Section& section) const;
    int codepointTextHeight() const;
    QRect headerRect(int y) const;
    QRect cellRect(const Section& section, int y, int index) const;
    GlyphRenderData glyphRenderData(uint16_t size, uint32_t codepoint);
    void updateScrollRange();
    void pruneCache(const QSet<quint64>& visibleKeys);

    static QString formatCodepoint(uint32_t codepoint);
    static quint64 cacheKey(uint16_t size, uint32_t codepoint);

    epd_asset_font_asset_t            m_asset             = nullptr;
    QVector<Section>                m_sections;
    QHash<quint64, GlyphRenderData> m_cache;
    uint16_t                        m_selectedSize      = 0U;
    uint32_t                        m_selectedCodepoint = 0U;
};

LEKCO_END_NAMESPACE

#endif // !_FONTGLYPHGRIDWIDGET_HPP_
