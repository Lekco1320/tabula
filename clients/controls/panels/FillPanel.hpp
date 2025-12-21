/**
 * @file FillPanel.hpp
 * @brief Panel to configure and fill previewer in an specified color.
 * 
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2025-12-21
 * @license MIT
 */

#pragma once

#ifndef _FILLPANEL_HPP_
#define _FILLPANEL_HPP_

#include "common/Common.h"
#include "controls/panels/ControlPanel.hpp"

class QCheckBox;
class QPushButton;

LEKCO_BEGIN_NAMESPACE

class ColorButton;

class FillPanel
    : public ControlPanel
{
    Q_OBJECT

public:
    explicit FillPanel(const QString& title, QWidget* parent = nullptr);

private:
    DrawFunc drawFunc() const override;

    QCheckBox*   m_previewBtn;
    ColorButton* m_colorBtn;
    QPushButton* m_draw;
};

LEKCO_END_NAMESPACE

#endif // !_FILLPANEL_HPP_
