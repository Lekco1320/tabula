/**
 * @file FontWorkspace.hpp
 * @brief EGF font asset workspace for the tabula desktop client.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2026-05-13
 * @license MIT
 */

#pragma once

#ifndef _FONTWORKSPACE_HPP_
#define _FONTWORKSPACE_HPP_

#include <stdint.h>
#include <QString>
#include <QVector>
#include <epd_asset/font_asset.h>

#include "controls/workspaces/ResourceWorkspace.hpp"

class QLabel;
class QPushButton;

LEKCO_BEGIN_NAMESPACE

class FontGlyphGridWidget;
class FontGlyphPreviewWidget;

class FontWorkspace
    : public ResourceWorkspace
{
    Q_OBJECT

public:
    explicit FontWorkspace(QWidget* parent = nullptr);
    ~FontWorkspace();

    void setResource(const ProjectResource& resource) override;
    void clearResource() override;
    QString resourcePath() const;

private:
    void loadResource();
    bool saveResource();
    void refreshSections();
    void selectGlyph(uint16_t size, uint32_t codepoint);
    void clearGlyphSelection();
    void addGlyph();
    void deleteGlyph();
    void setDetailsEnabled(bool enabled);
    void updateGlyphDetails(uint16_t size, uint32_t codepoint);

    static QString formatCodepoint(uint32_t codepoint);

    ProjectResource         m_resource;
    epd_asset_font_asset_t    m_asset             = nullptr;
    uint16_t                m_selectedSize      = 0U;
    uint32_t                m_selectedCodepoint = 0U;
    FontGlyphGridWidget*    m_grid              = nullptr;
    FontGlyphPreviewWidget* m_previewWidget     = nullptr;
    QLabel*                 m_codepointLabel    = nullptr;
    QLabel*                 m_metricsLabel      = nullptr;
    QPushButton*            m_addButton         = nullptr;
    QPushButton*            m_deleteButton      = nullptr;
};

LEKCO_END_NAMESPACE

#endif // !_FONTWORKSPACE_HPP_
