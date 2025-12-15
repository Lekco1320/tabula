/**
 * @file ColorButton.cpp
 * @brief Toggle button for switching colors.
 * 
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2025-12-13
 * @license MIT
 */

#include <QPixmap>
#include <QIcon>

#include "ColorButton.hpp"

LEKCO_BEGIN_NAMESPACE

QColor ColorButton::s_colors[] = { QColor(Qt::black), QColor(Qt::red), QColor(Qt::white) };

ColorButton::ColorButton(QWidget* parent)
    : QToolButton(parent)
    , m_index(0)
{
    setAutoRaise(false);
    setCheckable(false);
    setFixedSize(24, 24);
    setStyleSheet(
        "QToolButton { border: 1px solid #a0a0a0; border-radius: 3px; padding: 0; }"
        "QToolButton:pressed { border: 1px solid #707070; }");

    updateIcon();
    connect(this, &QToolButton::clicked, this, &ColorButton::nextColor);
}

QColor ColorButton::currentColor() const
{
    return s_colors[m_index];
}

void ColorButton::updateIcon()
{
    QPixmap pm(20, 20);
    pm.fill(currentColor());
    setIcon(QIcon(pm));
    setIconSize(pm.size());
}

void ColorButton::nextColor()
{
    m_index = (m_index + 1) % 3;
    updateIcon();
    emit colorChanged(currentColor());
}

LEKCO_END_NAMESPACE
