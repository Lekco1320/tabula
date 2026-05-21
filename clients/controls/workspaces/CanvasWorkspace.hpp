/**
 * @file CanvasWorkspace.hpp
 * @brief Canvas preview workspace for the tabula desktop client.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2026-05-13
 * @license MIT
 */

#pragma once

#ifndef _CANVASWORKSPACE_HPP_
#define _CANVASWORKSPACE_HPP_

#include <QWidget>
#include <epd_gfx/canvas.h>

#include "common/Common.h"

LEKCO_BEGIN_NAMESPACE

constexpr int kWorkspaceDetailsPaneWidth = 270;

class BitmapProvider;
class CanvasPreviewer;
class FontProvider;
class RotationBar;
class ToolPanel;

class CanvasWorkspace
    : public QWidget
{
    Q_OBJECT

public:
    explicit CanvasWorkspace(const epd_gfx_canvas_config_t& config, FontProvider* fontProvider,
        BitmapProvider* bitmapProvider, QWidget* parent = nullptr);

    void refreshProjectResources();

private:
    epd_gfx_canvas_config_t m_canvasConfig;
    ToolPanel*             m_toolPanel   = nullptr;
    CanvasPreviewer*       m_previewer   = nullptr;
    RotationBar*           m_rotationBar = nullptr;
};

LEKCO_END_NAMESPACE

#endif // !_CANVASWORKSPACE_HPP_
