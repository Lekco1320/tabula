/**
 * @file ColorButton.cpp
 * @brief Toggle button for switching colors.
 * 
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2025-12-13
 * @license MIT
 */

#include <QIcon>
#include <QPainter>
#include <QPixmap>

#include "controls/Utils.hpp"
#include "controls/widgets/ColorButton.hpp"

LEKCO_BEGIN_NAMESPACE

QColor ColorButton::s_colors[] = { QColor(Qt::black), QColor(Qt::red), QColor(Qt::white) };

ColorButton::ColorButton(QWidget* parent)
    : ColorButton(Mode::Foreground, parent)
{
}

ColorButton::ColorButton(Mode mode, QWidget* parent)
    : QToolButton(parent)
    , m_mode(mode)
    , m_index(0)
{
    SetupIconToolButton(this);

    updateIcon();
    connect(this, &QToolButton::clicked, this, &ColorButton::nextColor);
}

epd_gfx_color_t ColorButton::currentColor() const
{
    return QColorToEpdColor(s_colors[m_index]);
}

epd_gfx_bg_color_t ColorButton::currentBackgroundColor() const
{
    if (m_index == 0) {
        return EPD_GFX_BG_TRANSPARENT;
    }

    return static_cast<epd_gfx_bg_color_t>(QColorToEpdColor(s_colors[m_index - 1]));
}

void ColorButton::updateIcon()
{
    QPixmap pm(20, 20);
    pm.fill(Qt::transparent);
    QPainter painter(&pm);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRect rect(1, 1, 18, 18);
    if (m_mode == Mode::Background && m_index == 0) {
        painter.fillRect(rect, Qt::white);
        painter.setPen(QPen(Qt::red, 2));
        painter.drawLine(rect.bottomLeft() + QPoint(2, -2),
            rect.topRight() + QPoint(-2, 2));
    } else {
        const int index = (m_mode == Mode::Background) ? m_index - 1 : m_index;
        painter.fillRect(rect, s_colors[index]);
    }
    setIcon(QIcon(pm));
    setIconSize(pm.size());
}

void ColorButton::nextColor()
{
    m_index = (m_index + 1) % (m_mode == Mode::Background ? 4 : 3);
    updateIcon();
    if (m_mode == Mode::Background) {
        emit backgroundColorChanged(currentBackgroundColor());
    } else {
        emit colorChanged(currentColor());
    }
}

LEKCO_END_NAMESPACE
