/**
 * @file TextPanel.hpp
 * @brief Panel to configure and draw text.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2026-05-16
 * @license MIT
 */

#pragma once

#ifndef _TEXTPANEL_HPP_
#define _TEXTPANEL_HPP_

#include <QString>
#include <QPlainTextEdit>

#include "controls/panels/ControlPanel.hpp"
#include "project/FontProvider.hpp"

class QCheckBox;
class QComboBox;
class QFocusEvent;
class QKeyEvent;
class QMimeData;
class QPushButton;
class QSpinBox;

LEKCO_BEGIN_NAMESPACE

class ColorButton;
class FlowButton;

class SingleLineTextEdit
    : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit SingleLineTextEdit(QWidget* parent = nullptr);

protected:
    void insertFromMimeData(const QMimeData* source) override;
    void keyPressEvent(QKeyEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;

private:
    void normalizeText();
};

class TextPanel
    : public ControlPanel
{
    Q_OBJECT

public:
    explicit TextPanel(const QString& title, FontProvider* fontProvider, QWidget* parent = nullptr);

    void updatePreview() const override;
    void updateRange(epd_gfx_canvas_t canvas) override;
    void refreshProjectResources() override;

private:
    DrawFunc drawFunc() const override;

    bool canUseTool() const;
    bool canDraw() const;
    void refreshFonts();
    void refreshSizes();
    void refreshTextRenderable();
    void updateControls();

    FontProvider*       m_fontProvider = nullptr;
    QComboBox*          m_font         = nullptr;
    QComboBox*          m_size         = nullptr;
    SingleLineTextEdit* m_text         = nullptr;
    QSpinBox*           m_x            = nullptr;
    QSpinBox*           m_y            = nullptr;
    ColorButton*        m_colorBtn     = nullptr;
    ColorButton*        m_background   = nullptr;
    FlowButton*         m_flow         = nullptr;
    QSpinBox*           m_spacing      = nullptr;
    QCheckBox*          m_previewBtn   = nullptr;
    QPushButton*        m_draw         = nullptr;
    bool                m_renderable   = true;
};

LEKCO_END_NAMESPACE

#endif // !_TEXTPANEL_HPP_
