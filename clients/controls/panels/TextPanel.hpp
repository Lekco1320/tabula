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
class QMimeData;
class QPushButton;
class QSpinBox;

LEKCO_BEGIN_NAMESPACE

class ColorButton;
class TextAlignButton;
class TextWrapButton;

class TextBoxEdit
    : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit TextBoxEdit(QWidget* parent = nullptr);

protected:
    void insertFromMimeData(const QMimeData* source) override;
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

    void connectSignals();
    void updateTextState();
    void updatePosition();
    bool canUseTool() const;
    bool canDraw() const;
    void refreshFonts();
    void refreshSizes();
    void refreshTextRenderable();
    void updateBoxRange(bool expandToMax = false);
    void updateControls();

    FontProvider*    m_fontProvider = nullptr;
    QComboBox*       m_font         = nullptr;
    QComboBox*       m_size         = nullptr;
    TextBoxEdit*     m_text         = nullptr;
    QSpinBox*        m_x            = nullptr;
    QSpinBox*        m_y            = nullptr;
    QSpinBox*        m_width        = nullptr;
    QSpinBox*        m_height       = nullptr;
    QSpinBox*        m_lineSpacing  = nullptr;
    QSpinBox*        m_charSpacing  = nullptr;
    ColorButton*     m_foreground   = nullptr;
    ColorButton*     m_background   = nullptr;
    TextAlignButton* m_align        = nullptr;
    TextWrapButton*  m_wrap         = nullptr;
    QCheckBox*       m_previewBtn   = nullptr;
    QPushButton*     m_draw         = nullptr;
    bool             m_renderable   = true;
};

LEKCO_END_NAMESPACE

#endif // !_TEXTPANEL_HPP_
