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
#include <QSizePolicy>
#include <QToolButton>
#include <QVector>
#include <epd_gfx/codec.h>

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

bool FrameViewToQImage(const epd_gfx_frame_view_t* view, QImage* outImage)
{
    if (!view || !outImage || view->width == 0U || view->height == 0U) {
        return false;
    }

    QImage image(view->width, view->height, QImage::Format_RGB888);
    QVector<epd_gfx_color_t> colors(view->width);

    switch (view->format) {
    case EPD_GFX_FORMAT_NATIVE:
        if (!view->buf_native || view->stride < epd_gfx_native_stride(view->width)) {
            return false;
        }
        for (uint16_t y = 0U; y < view->height; ++y) {
            const uint8_t* row = view->buf_native + (uint32_t)y * view->stride;
            if (epd_gfx_native_to_color_buffer(row, view->width, colors.data()) != EPD_OK) {
                return false;
            }
            for (uint16_t x = 0U; x < view->width; ++x) {
                image.setPixelColor(x, y, EpdColorToQColor(colors[x]));
            }
        }
        *outImage = image;
        return true;

    case EPD_GFX_FORMAT_PLANES:
        if (!view->buf_wht || !view->buf_red || view->stride < epd_gfx_planes_stride(view->width)) {
            return false;
        }
        for (uint16_t y = 0U; y < view->height; ++y) {
            const uint8_t* rowWht = view->buf_wht + (uint32_t)y * view->stride;
            const uint8_t* rowRed = view->buf_red + (uint32_t)y * view->stride;
            if (epd_gfx_planes_to_color_buffer(rowWht, rowRed, view->width, colors.data()) != EPD_OK) {
                return false;
            }
            for (uint16_t x = 0U; x < view->width; ++x) {
                image.setPixelColor(x, y, EpdColorToQColor(colors[x]));
            }
        }
        *outImage = image;
        return true;

    default:
        return false;
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

void SetupIconToolButton(QToolButton* button, bool persistent)
{
    if (!button) {
        return;
    }

    button->setAutoRaise(false);
    button->setCheckable(false);
    button->setFixedSize(28, 28);
    button->setFocusPolicy(Qt::NoFocus);
    button->setIconSize(QSize(20, 20));
    button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    button->setStyleSheet(persistent
        ? QStringLiteral(
            "QToolButton { background: #f1f3f5; border: 1px solid #d5d9de; border-radius: 6px; padding: 0; }"
            "QToolButton:hover { background: #e9ecef; border-color: #c8cdd3; }"
            "QToolButton:pressed { background: #dde2e7; border-color: #aeb6bf; }"
            "QToolButton:disabled { background: transparent; border-color: transparent; }")
        : QStringLiteral(
            "QToolButton { background: transparent; border: 1px solid transparent; border-radius: 6px; padding: 0; }"
            "QToolButton:hover { background: #eef1f4; border-color: #d9dee4; }"
            "QToolButton:pressed { background: #dde2e7; border-color: #bdc5ce; }"
            "QToolButton:disabled { background: transparent; border-color: transparent; }"));
}

LEKCO_END_NAMESPACE
