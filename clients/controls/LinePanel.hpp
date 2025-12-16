/**
 * @file LinePanel.hpp
 * @brief Panel to configure and draw a line.
 * 
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2025-12-16
 * @license MIT
 */

#pragma once

#include <QWidget>
#include <QColor>

#include "common.h"
#include "ControlPanel.hpp"

class QSpinBox;
class QPushButton;

LEKCO_BEGIN_NAMESPACE

class ColorButton;

class LinePanel
    : public ControlPanel
{
    Q_OBJECT

public:
    explicit LinePanel(CanvasPreviewer* previewer, Qt::Orientation orientation, QWidget* parent = nullptr);

private:
    void updateRange();
    void flushTo(epd_gfx_canvas_t canvas) override;

    Qt::Orientation m_orientation;
    QSpinBox*       m_x;
    QSpinBox*       m_y;
    QSpinBox*       m_len;
    ColorButton*    m_colorBtn;
    QPushButton*    m_draw;
};

LEKCO_END_NAMESPACE
