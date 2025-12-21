/**
 * @file Utils.cpp
 * @brief Utilities for clients.
 * 
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2025-12-21
 * @license MIT
 */

#include <QGuiApplication>
#include <QScreen>

#include "controls/Utils.hpp"

LEKCO_BEGIN_NAMESPACE

QColor EpdColorToQColor(epd_gfx_color_t color)
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

epd_gfx_color_t QColorToEpdColor(QColor color)
{
    if (color == Qt::red) {
        return EPD_GFX_RED;
    } else if (color == Qt::black) {
        return EPD_GFX_BLACK;
    } else {
        return EPD_GFX_WHITE;
    }
}

void SetWindowCenterScreen(QWidget* window)
{
    QScreen* screen = QGuiApplication::primaryScreen();
    QRect screenRect = screen->geometry(); 

    int x = (screenRect.width() - window->width()) / 2;
    int y = (screenRect.height() - window->height()) / 2;
    window->move(x, y);
}

LEKCO_END_NAMESPACE
