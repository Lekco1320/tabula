/**
 * @file PixelPanel.hpp
 * @brief Panel to configure and draw a pixel.
 * 
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2025-12-21
 * @license MIT
 */

#pragma once

#ifndef _PIXELPANEL_HPP_
#define _PIXELPANEL_HPP_

#include "controls/panels/ControlPanel.hpp"

#include "common/Common.h"

class QSpinBox;
class QCheckBox;
class QPushButton;

LEKCO_BEGIN_NAMESPACE

class ColorButton;

class PixelPanel
    : public ControlPanel
{
    Q_OBJECT

public:
    explicit PixelPanel(const QString& title, QWidget* parent = nullptr);

    void updateRange(const epd_gfx_canvas_t canvas) override;

private:
    DrawFunc drawFunc() const override;

    QSpinBox*    m_x;
    QSpinBox*    m_y;
    QCheckBox*   m_previewBtn;
    ColorButton* m_colorBtn;
    QPushButton* m_draw;
};

LEKCO_END_NAMESPACE

#endif // !_PIXELPANEL_HPP_
