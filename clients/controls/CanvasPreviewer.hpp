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
#include <QImage>
#include <QPointF>
#include <QColor>
#include <QRectF>
#include <QPoint>

#include "common.h"
#include "epd_gfx/canvas.h"

class QMouseEvent;
class QEvent;
class QEnterEvent;
class QPainter;

LEKCO_BEGIN_NAMESPACE

class CanvasPreviewer
    : public QWidget
{
    Q_OBJECT

public:
    enum class Cursor { Pointer, Inspect };

    explicit CanvasPreviewer(epd_gfx_canvas_config_t config, QWidget* parent);
    ~CanvasPreviewer();
    
    // Canvas access
    epd_gfx_canvas_t getCanvas() const;
    void refresh();
    void setRotation(epd_gfx_rotation_t rotation);
    void setCursor(Cursor mode);
    void setPreviewCanvas(epd_gfx_canvas_t preview);

signals:
    void rotationChanged(epd_gfx_rotation_t rotation);

protected:
    void paintEvent(QPaintEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    struct HoverInfo {
        bool    active = false;
        QPointF widgetPos;
        QPointF imagePos;
        QRectF  targetRect;
        QColor  color;
    };

    // Helpers
    bool       mapToImage(const QPointF& widgetPos, QPointF& imagePos, QRectF& targetRect) const;
    QPoint     mapToLogical(const QPoint& base) const;
    HoverInfo  currentHoverInfo() const;
    void       drawCanvasImage(QPainter& painter, const QRectF& content, const HoverInfo& hover) const;
    void       drawOverlay(QPainter& painter, const QRectF& content, const HoverInfo& hover) const;
    void       startRotationAnimation(epd_gfx_rotation_t target);
    void       rebuildImage(epd_gfx_canvas_t canvas);
    epd_err_t  rebuildImageFromNative(const uint8_t* data, uint32_t size);
    epd_err_t  rebuildImageFromPlanes(const uint8_t* pwht, const uint8_t* pred, uint32_t size);

    // Static helpers
    static epd_err_t flushImpl(void* ctx, const epd_gfx_frame_view_t* view);

    // State
    epd_gfx_canvas_config_t m_config;
    epd_gfx_canvas_t        m_canvas;

    qreal  m_angleFrom;
    qreal  m_angleTo;
    qreal  m_angleCurrent;
    QImage m_baseImage;   // rotation-0 image

    Cursor  m_cursor;
    bool    m_hasMouse;
    QPointF m_lastMousePos;
};

LEKCO_END_NAMESPACE

#endif // !_CANVASPREVIEWER_H_
