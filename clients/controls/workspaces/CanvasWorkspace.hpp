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

#include <epd_gfx/canvas.h>

#include "controls/workspaces/ResourceWorkspace.hpp"

LEKCO_BEGIN_NAMESPACE

class CanvasPreviewer;
class CursorBar;
class FontProvider;
class RotationBar;
class ToolPanel;

class CanvasWorkspace
    : public ResourceWorkspace
{
    Q_OBJECT

public:
    explicit CanvasWorkspace(const epd_gfx_canvas_config_t& config, FontProvider* fontProvider,
        QWidget* parent = nullptr);

    void setResource(const ProjectResource& resource) override;
    void clearResource() override;
    void refreshFonts();

private:
    epd_gfx_canvas_config_t m_canvasConfig;
    CursorBar*             m_cursorBar   = nullptr;
    ToolPanel*             m_toolPanel   = nullptr;
    CanvasPreviewer*       m_previewer   = nullptr;
    RotationBar*           m_rotationBar = nullptr;
};

LEKCO_END_NAMESPACE

#endif // !_CANVASWORKSPACE_HPP_
