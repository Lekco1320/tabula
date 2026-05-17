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
#include <QWidget>
#include <epd_asset/font_asset.h>

#include "project/Project.hpp"

class QLabel;
class QPushButton;
class QStackedWidget;

LEKCO_BEGIN_NAMESPACE

class FontGlyphGridWidget;
class FontGlyphPreviewWidget;

class FontWorkspace
    : public QWidget
{
    Q_OBJECT

public:
    explicit FontWorkspace(const Project& project, QWidget* parent = nullptr);
    ~FontWorkspace();

    void setResource(const ProjectResource& resource);
    void setFontResources(const QVector<ProjectResource>& resources);
    void clearResource();
    QString resourcePath() const;

private:
    enum class SelectionKind {
        None,
        Size,
        Glyph,
    };

    void loadResource();
    void clearLoadedFont();
    bool saveResource();
    void refreshSections();
    void selectSize(uint16_t size);
    void selectGlyph(uint16_t size, uint32_t codepoint);
    void clearGlyphSelection();
    void addGlyph();
    void deleteSelected();
    void deleteSize();
    void deleteGlyph();
    bool hasEditableSource() const;
    void setDetailsEnabled(bool enabled);
    void updateFontSummary();
    void updateFontResourcesSummary(const QVector<ProjectResource>& resources);
    void updateSizeSummary(uint16_t size);
    void updateGlyphDetails(uint16_t size, uint32_t codepoint);

    static QString formatCodepoint(uint32_t codepoint);

    const Project&          m_project;
    ProjectResource         m_resource;
    epd_asset_font_asset_t  m_asset             = nullptr;
    SelectionKind           m_selectionKind     = SelectionKind::None;
    uint16_t                m_selectedSize      = 0U;
    uint32_t                m_selectedCodepoint = 0U;
    QStackedWidget*         m_contentStack      = nullptr;
    QWidget*                m_resourcesPage     = nullptr;
    QLabel*                 m_resourcesTitle    = nullptr;
    QLabel*                 m_resourcesMetrics  = nullptr;
    QWidget*                m_editorPage        = nullptr;
    FontGlyphGridWidget*    m_grid              = nullptr;
    FontGlyphPreviewWidget* m_previewWidget     = nullptr;
    QLabel*                 m_titleLabel        = nullptr;
    QLabel*                 m_codepointLabel    = nullptr;
    QLabel*                 m_metricsLabel      = nullptr;
    QPushButton*            m_addButton         = nullptr;
    QPushButton*            m_deleteButton      = nullptr;
};

LEKCO_END_NAMESPACE

#endif // !_FONTWORKSPACE_HPP_
