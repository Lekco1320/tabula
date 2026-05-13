/**
 * @file AddGlyphDialog.hpp
 * @brief Dialog for adding glyphs to an EGF font asset.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2026-05-13
 * @license MIT
 */

#pragma once

#ifndef _ADDGLYPHDIALOG_HPP_
#define _ADDGLYPHDIALOG_HPP_

#include <stdint.h>
#include <QDialog>
#include <QString>

#include "common/Common.h"

class QLineEdit;
class QRadioButton;
class QSpinBox;
class QStackedWidget;

LEKCO_BEGIN_NAMESPACE

class AddGlyphDialog
    : public QDialog
{
    Q_OBJECT

public:
    explicit AddGlyphDialog(QWidget* parent = nullptr);

    QString fontPath() const;
    uint16_t size() const;
    uint32_t startCodepoint() const;
    uint32_t endCodepoint() const;

private:
    void accept() override;
    void browseFont();
    void updateMode();

    static bool parseCodepoint(const QString& text, uint32_t* out_codepoint);

    QLineEdit*      m_fontPathEdit        = nullptr;
    QSpinBox*       m_sizeSpin            = nullptr;
    QRadioButton*   m_singleRadio         = nullptr;
    QRadioButton*   m_rangeRadio          = nullptr;
    QLineEdit*      m_singleCodepoint     = nullptr;
    QLineEdit*      m_rangeStartCodepoint = nullptr;
    QLineEdit*      m_rangeEndCodepoint   = nullptr;
    QStackedWidget* m_codepointStack      = nullptr;
    uint32_t        m_startCodepoint      = 0U;
    uint32_t        m_endCodepoint        = 0U;
};

LEKCO_END_NAMESPACE

#endif // !_ADDGLYPHDIALOG_HPP_
