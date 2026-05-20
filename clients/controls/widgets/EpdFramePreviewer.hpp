/**
 * @file EpdFramePreviewer.hpp
 * @brief Scrollable tri-color EPD frame preview widget.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2026-05-20
 * @license MIT
 */

#pragma once

#ifndef _EPDFRAMEPREVIEWER_HPP_
#define _EPDFRAMEPREVIEWER_HPP_

#include <QColor>
#include <QImage>
#include <QPoint>
#include <QPointF>
#include <QWidget>
#include <epd_gfx/common.h>
#include <epd_gfx/frame_view.h>

#include "common/Common.h"

class QEvent;
class QLabel;
class QResizeEvent;
class QScrollArea;
class QToolButton;

LEKCO_BEGIN_NAMESPACE

class CycleIconButton;

class EpdFramePreviewer
    : public QWidget
{
    Q_OBJECT

public:
    enum class Cursor {
        Pointer,
        Inspect,
    };

    explicit EpdFramePreviewer(QWidget* parent = nullptr);

    bool setFrameView(const epd_gfx_frame_view_t* view);
    void setImage(const QImage& image);
    void clear();
    void setDisplayRotation(epd_gfx_rotation_t rotation);
    void setLogicalSize(uint16_t width, uint16_t height);

private:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

    class Overlay;
    class Viewport;

    void setCursorMode(Cursor cursor);
    void setCursorModeIndex(int index);
    void zoomIn();
    void zoomOut();
    void stepZoom(int direction, const QPoint* anchor = nullptr);
    void setZoomIndex(int index, const QPoint* anchor = nullptr);
    void updateZoomLabel();
    void updateHover(bool active, const QPointF& imagePos = QPointF(),
        const QPoint& logicalPoint = QPoint(), const QColor& color = QColor());

    Viewport*         m_viewport       = nullptr;
    Overlay*          m_overlay        = nullptr;
    QScrollArea*      m_scrollArea     = nullptr;
    CycleIconButton*  m_cursorButton   = nullptr;
    QToolButton*      m_zoomOutButton  = nullptr;
    QLabel*           m_zoomLabel      = nullptr;
    QToolButton*      m_zoomInButton   = nullptr;
    QImage            m_image;
    bool              m_hoverActive    = false;
    QPointF           m_hoverImagePos;
    QPoint            m_hoverLogicalPoint;
    QColor            m_hoverColor;
    int               m_zoomIndex      = 5;
};

LEKCO_END_NAMESPACE

#endif // !_EPDFRAMEPREVIEWER_HPP_
