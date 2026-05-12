/**
 * @file PreviewWindow.hpp
 * @brief Canvas preview window for the tabula desktop client.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2026-05-12
 * @license MIT
 */

#pragma once

#ifndef _PREVIEWWINDOW_HPP_
#define _PREVIEWWINDOW_HPP_

#include <QMainWindow>
#include <epd_gfx/canvas.h>

#include "common/Common.h"
#include "controls/widgets/CanvasPreviewer.hpp"

LEKCO_BEGIN_NAMESPACE

class CanvasPreviewer;
class CursorBar;
class RotationBar;
class ToolPanel;
class AdaptiveStackedWidget;

class PreviewWindow
    : public QMainWindow
{
    Q_OBJECT

public:
    explicit PreviewWindow(const epd_gfx_canvas_config_t& config, QWidget* parent = nullptr);

private:
    epd_gfx_canvas_config_t m_canvasConfig;

    CursorBar*             m_cursorBar     = nullptr;
    ToolPanel*             m_toolPanel     = nullptr;
    CanvasPreviewer*       m_previewer     = nullptr;
    AdaptiveStackedWidget* m_stackedWidget = nullptr;
    RotationBar*           m_rotationBar   = nullptr;
};

LEKCO_END_NAMESPACE

#endif // !_PREVIEWWINDOW_HPP_
