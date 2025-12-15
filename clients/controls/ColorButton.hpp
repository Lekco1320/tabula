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

#include "common.h"

#include <QToolButton>
#include <QColor>
#include <QVector>

LEKCO_BEGIN_NAMESPACE

class ColorButton
    : public QToolButton
{
    Q_OBJECT

public:
    explicit ColorButton(QWidget* parent = nullptr);

    QColor currentColor() const;

signals:
    void colorChanged(const QColor& color);

private:
    void updateIcon();
    void nextColor();

    int m_index;
    static QColor s_colors[];
};

LEKCO_END_NAMESPACE

#endif // _COLORBUTTON_HPP_