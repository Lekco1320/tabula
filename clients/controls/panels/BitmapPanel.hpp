/**
 * @file BitmapPanel.hpp
 * @brief Panel to configure and draw a bitmap.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2026-05-21
 * @license MIT
 */

#pragma once

#ifndef _BITMAPPANEL_HPP_
#define _BITMAPPANEL_HPP_

#include "controls/panels/ControlPanel.hpp"

class QCheckBox;
class QComboBox;
class QLabel;
class QPushButton;
class QSpinBox;

LEKCO_BEGIN_NAMESPACE

class BitmapProvider;

class BitmapPanel
    : public ControlPanel
{
    Q_OBJECT

public:
    explicit BitmapPanel(const QString& title, BitmapProvider* bitmapProvider, QWidget* parent = nullptr);

    void updatePreview() const override;
    void updateRange(epd_gfx_canvas_t canvas) override;
    void refreshProjectResources() override;

private:
    DrawFunc drawFunc() const override;

    void refreshBitmaps();
    void updateBitmapState();
    void updateControls();
    bool canDraw() const;

    BitmapProvider* m_bitmapProvider = nullptr;
    QComboBox*      m_bitmap         = nullptr;
    QLabel*         m_size           = nullptr;
    QSpinBox*       m_x              = nullptr;
    QSpinBox*       m_y              = nullptr;
    QCheckBox*      m_previewBtn     = nullptr;
    QPushButton*    m_draw           = nullptr;
};

LEKCO_END_NAMESPACE

#endif // !_BITMAPPANEL_HPP_
