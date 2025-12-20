/**
 * @file LinePanel.hpp
 * @brief Panel to configure and draw a line.
 * 
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2025-12-16
 * @license MIT
 */

#pragma once

#ifndef _LINEPANEL_HPP_
#define _LINEPANEL_HPP_

#include <QString>
#include <QWidget>
#include <QColor>

#include "common/common.h"
#include "controls/panels/ControlPanel.hpp"

class QSpinBox;
class QPushButton;

LEKCO_BEGIN_NAMESPACE

class ColorButton;

class LinePanel
    : public ControlPanel
{
    Q_OBJECT

public:
    explicit LinePanel(const QString& title, Qt::Orientation orientation,
        QWidget* parent = nullptr);

    void updateRange(const epd_gfx_canvas_t canvas) override;

private:
    DrawFunc drawFunc() const override;

    Qt::Orientation m_orientation;
    QSpinBox*       m_x;
    QSpinBox*       m_y;
    QSpinBox*       m_len;
    ColorButton*    m_colorBtn;
    QCheckBox*      m_previewBtn;
    QPushButton*    m_draw;
};

LEKCO_END_NAMESPACE

#endif // !_LINEPANEL_HPP_
