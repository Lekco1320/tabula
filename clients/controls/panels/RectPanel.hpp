/**
 * @file RectPanel.hpp
 * @brief Panel to draw or fill a rectangle.
 * 
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2025-12-19
 * @license MIT
 */

#pragma once

#ifndef _RECTPANEL_HPP_
#define _RECTPANEL_HPP_

#include "controls/panels/ControlPanel.hpp"

#include "common/common.h"

class QSpinBox;
class QCheckBox;
class QPushButton;

LEKCO_BEGIN_NAMESPACE

class ColorButton;

class RectPanel
    : public ControlPanel
{
    Q_OBJECT

public:
    explicit RectPanel(const QString& title, bool isDraw, QWidget* parent = nullptr);

    void updateRange(const epd_gfx_canvas_t canvas) override;

private:
    DrawFunc drawFunc() const override;

    bool         m_isDraw;
    QSpinBox*    m_x;
    QSpinBox*    m_y;
    QSpinBox*    m_width;
    QSpinBox*    m_height;
    ColorButton* m_colorBtn;
    QCheckBox*   m_previewBtn;
    QPushButton* m_draw;
};

LEKCO_END_NAMESPACE

#endif // !_RECTPANEL_HPP_
