/**
 * @file Utils.hpp
 * @brief Utilities for clients.
 * 
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2025-12-16
 * @license MIT
 */

#pragma once 

#ifndef _UTILS_HPP_
#define _UTILS_HPP_

#include <QLabel>
#include <QColor>
#include <QWidget>
#include <QHBoxLayout>
#include <type_traits>
#include <epd_gfx/common.h>
#include <epd_gfx/canvas.h>

#include "common/common.h"

LEKCO_BEGIN_NAMESPACE

using DrawFunc = std::function<epd_err_t(epd_gfx_canvas_t)>;

static inline QColor EpdColorToQColor(epd_gfx_color_t color)
{
    static QColor white = QRgb{ 0xFFFFFF };
    static QColor black = QRgb{ 0x000000 };
    static QColor red   = QRgb{ 0xFF0000 };

    switch (color)
    {
    case EPD_GFX_BLACK:
        return black;

    case EPD_GFX_RED:
        return red;

    default:
        return white;
    }
}

static inline epd_gfx_color_t QColorToEpdColor(QColor color)
{
    if (color == Qt::red) {
        return EPD_GFX_RED;
    } else if (color == Qt::black) {
        return EPD_GFX_BLACK;
    } else {
        return EPD_GFX_WHITE;
    }
}

template <typename... Widgets>
std::enable_if_t<(std::is_base_of_v<QWidget, std::remove_pointer_t<Widgets>> && ...), QWidget*>
MakeRow(QWidget* parent, int spacing, Widgets*... widgets)
{
    auto* row    = new QWidget(parent);
    auto* layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(spacing);
    (layout->addWidget(widgets), ...);
    return row;
}

template <typename Widget>
std::enable_if_t<std::is_base_of_v<QWidget, Widget>, QWidget*>
MakeLabeledWidget(QWidget* parent, const QString& label, Widget* widget, int stretch = 1)
{
    auto* w      = new QWidget(parent);
    auto* layout = new QHBoxLayout(w);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(new QLabel(label, w));
    layout->addWidget(widget, stretch, stretch ? Qt::Alignment{} : Qt::AlignLeft);
    return w;
}

template <typename Widget>
std::enable_if_t<std::is_base_of_v<QWidget, Widget>, QWidget*>
MakeLabeledWidget(QWidget* parent, Widget* widget, const QString& label, int stretch = 0)
{
    auto* w      = new QWidget(parent);
    auto* layout = new QHBoxLayout(w);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(widget);
    layout->addWidget(new QLabel(label, w), stretch, stretch ? Qt::Alignment{} : Qt::AlignLeft);
    return w;
}

LEKCO_END_NAMESPACE

#endif // !_UTILS_HPP_
