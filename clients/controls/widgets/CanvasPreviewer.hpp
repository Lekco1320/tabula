/**
 * @file CanvasPreviewer.hpp
 * @brief Tri-color previewer for `epd_canvas_t`.
 * 
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2025-12-10
 * @license MIT
 */

#pragma once

#ifndef _CANVASPREVIEWER_H_
#define _CANVASPREVIEWER_H_

#include <QWidget>
#include <epd_gfx/canvas.h>

#include "common/Common.h"
#include "controls/Utils.hpp"

LEKCO_BEGIN_NAMESPACE

class EpdFramePreviewer;

class CanvasPreviewer
    : public QWidget
{
    Q_OBJECT

public:
    explicit CanvasPreviewer(epd_gfx_canvas_config_t config, QWidget* parent);
    ~CanvasPreviewer();
    
    epd_gfx_canvas_t getCanvas() const;
    void refresh();
    void setRotation(epd_gfx_rotation_t rotation);
    void drawCanvas(const DrawFunc& drawFunc);
    void drawPreview(const DrawFunc& drawFunc);

private:
    void rebuildImage(epd_gfx_canvas_t canvas);

    static epd_err_t flushImpl(void* ctx, const epd_gfx_frame_view_t* view);

    epd_gfx_canvas_config_t m_config;
    epd_gfx_canvas_t        m_canvas         = nullptr;
    EpdFramePreviewer*      m_framePreviewer = nullptr;

signals:
    void rotationChanged(epd_gfx_rotation_t rotation);
    void errorOccurred(const QString& message);
};

LEKCO_END_NAMESPACE

#endif // !_CANVASPREVIEWER_H_
