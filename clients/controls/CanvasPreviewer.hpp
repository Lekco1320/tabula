/**
 * @file CanvasPreviewer.h
 * @brief Tri-color previewer for `epd_canvas`.
 * 
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2025-12-10
 * @license MIT
 */

#pragma once

#ifndef _CANVASPREVIEWER_H_
#define _CANVASPREVIEWER_H_

#include <QWidget>
#include <QImage>

#include "common.h"
#include "epd_gfx/canvas.h"

_LEKCO_BEGIN_NAMESPACE

class CanvasPreviewer
    : public QWidget
{
    Q_OBJECT

public:
    explicit CanvasPreviewer(epd_gfx_canvas_config_t config, QWidget* parent);
    ~CanvasPreviewer();

    epd_gfx_canvas_t getCanvas() const;
    void refresh();
    void loadNativeBuffer(const uint8_t* data, uint32_t size);
    void loadPlanesBuffer(const uint8_t* pwht, const uint8_t* pred, uint32_t size);
    void setRotation(epd_gfx_rotation_t rotation);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void startRotationAnimation(epd_gfx_rotation_t target);
    void rebuildImage();
    epd_err_t rebuildImageFromNative(const uint8_t* data, uint32_t size);
    epd_err_t rebuildImageFromPlanes(const uint8_t* pwht, const uint8_t* pred, uint32_t size);
    static epd_err_t flushImpl(void* ctx, const epd_gfx_frame_view_t* view);

    epd_gfx_canvas_config_t m_config;
    epd_gfx_canvas_t        m_canvas;

    qreal  m_angleFrom;
    qreal  m_angleTo;
    qreal  m_angleCurrent;
    QImage m_baseImage;   // rotation-0 image
};

_LEKCO_END_NAMESPACE

#endif // !_CANVASPREVIEWER_H_
