/**
 * @file ColorButton.hpp
 * @brief Toggle button for switching colors.
 * 
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2025-12-13
 * @license MIT
 */

#pragma once

#ifndef _COLORBUTTON_HPP_
#define _COLORBUTTON_HPP_

#include <QToolButton>
#include <QColor>
#include <QVector>
#include <epd_gfx/common.h>

#include "common/Common.h"

LEKCO_BEGIN_NAMESPACE

class ColorButton
    : public QToolButton
{
    Q_OBJECT

public:
    enum class Mode {
        Foreground,
        Background,
    };

    explicit ColorButton(QWidget* parent = nullptr);
    explicit ColorButton(Mode mode, QWidget* parent = nullptr);

    epd_gfx_color_t currentColor() const;
    epd_gfx_bg_color_t currentBackgroundColor() const;

signals:
    void colorChanged(epd_gfx_color_t color);
    void backgroundColorChanged(epd_gfx_bg_color_t color);

private:
    void updateIcon();
    void nextColor();

    Mode m_mode;
    int  m_index;
    static QColor s_colors[];
};

LEKCO_END_NAMESPACE

#endif // _COLORBUTTON_HPP_
