/**
 * @file BitmapWorkspace.hpp
 * @brief EBM bitmap asset workspace for the tabula desktop client.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2026-05-20
 * @license MIT
 */

#pragma once

#ifndef _BITMAPWORKSPACE_HPP_
#define _BITMAPWORKSPACE_HPP_

#include <QString>
#include <QWidget>

#include "project/Project.hpp"

class QComboBox;
class QLabel;
class QSpinBox;
class QStackedWidget;

LEKCO_BEGIN_NAMESPACE

class EpdFramePreviewer;

class BitmapWorkspace
    : public QWidget
{
    Q_OBJECT

public:
    explicit BitmapWorkspace(Project& project, QWidget* parent = nullptr);

    void setResource(const ProjectResource& resource);
    void setBitmapResources(const QVector<ProjectResource>& resources);
    void clearResource();
    QString resourcePath() const;

private:
    void loadResource();
    void loadPreview();
    void syncControls();
    void setControlsEnabled(bool enabled);
    void updateBitmapSummary();
    void updateBitmapResourcesSummary(const QVector<ProjectResource>& resources);
    void updateAlgorithm();
    void updateAlgorithmPage();
    void autoApply();
    ProjectBitmapInfo infoFromControls() const;
    bool restoreBitmap(const ProjectBitmapInfo& info);

    static QString formatName(epd_gfx_format_t format);

    Project&           m_project;
    ProjectResource    m_resource;
    ProjectBitmapInfo  m_info;
    bool               m_hasInfo          = false;
    bool               m_editable         = false;
    bool               m_updating         = false;
    QStackedWidget*    m_contentStack     = nullptr;
    QWidget*           m_resourcesPage    = nullptr;
    QLabel*            m_resourcesTitle   = nullptr;
    QLabel*            m_resourcesMetrics = nullptr;
    QWidget*           m_editorPage       = nullptr;
    EpdFramePreviewer* m_previewer        = nullptr;
    QLabel*            m_titleLabel       = nullptr;
    QLabel*            m_statusLabel      = nullptr;
    QLabel*            m_metricsLabel     = nullptr;
    QComboBox*         m_algorithm        = nullptr;
    QStackedWidget*    m_paramStack       = nullptr;
    QSpinBox*          m_blackThreshold   = nullptr;
    QSpinBox*          m_redThreshold     = nullptr;
    QSpinBox*          m_redSaturation    = nullptr;
    QComboBox*         m_orderedMatrix    = nullptr;
    QComboBox*         m_blueNoiseMatrix  = nullptr;
};

LEKCO_END_NAMESPACE

#endif // !_BITMAPWORKSPACE_HPP_
