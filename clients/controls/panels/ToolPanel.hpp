/**
 * @file ToolPanel.hpp
 * @brief Tool panel for MainWindow.
 * 
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2025-12-18
 * @license MIT
 */

#pragma once

#ifndef _TOOLPANEL_HPP_
#define _TOOLPANEL_HPP_

#include <QWidget>
#include <QVector>

#include "common/Common.h"
#include "controls/panels/ControlPanel.hpp"

LEKCO_BEGIN_NAMESPACE

class ToolBar;
class CanvasPreviewer;
class AdaptiveStackedWidget;

class ToolPanel
    : public QWidget
{
    Q_OBJECT

public:
    explicit ToolPanel(CanvasPreviewer* previewer, QWidget* parent = nullptr);
    void updateCanvas(const epd_gfx_canvas_t canvas);

signals:
    void refreshRequested();
    void drawRequested(const DrawFunc& drawFunc);
    void previewRequested(const DrawFunc& drawFunc);

private:
    CanvasPreviewer*       m_previewer;
    ToolBar*               m_toolBar;
    QVector<ControlPanel*> m_controlPanels;
    AdaptiveStackedWidget* m_stackedWidget;

    void addControlPanel(ControlPanel* panel);
};

LEKCO_END_NAMESPACE

#endif // !_TOOLPANEL_HPP_
