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

#include <functional>
#include <type_traits>
#include <QImage>
#include <QLabel>
#include <QColor>
#include <QWidget>
#include <QHBoxLayout>
#include <epd_gfx/common.h>
#include <epd_gfx/canvas.h>

#include "common/Common.h"

class QToolButton;

LEKCO_BEGIN_NAMESPACE

using DrawFunc = std::function<epd_err_t(epd_gfx_canvas_t)>;

QColor EpdColorToQColor(epd_gfx_color_t color);

epd_gfx_color_t QColorToEpdColor(QColor color);

bool FrameViewToQImage(const epd_gfx_frame_view_t* view, QImage* outImage);

void SetWindowCenterScreen(QWidget* window);

void SetupIconToolButton(QToolButton* button, bool persistent = false);

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
