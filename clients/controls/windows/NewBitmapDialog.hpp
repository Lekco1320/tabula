/**
 * @file NewBitmapDialog.hpp
 * @brief New EBM bitmap dialog for project assets.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2026-05-20
 * @license MIT
 */

#pragma once

#ifndef _NEWBITMAPDIALOG_HPP_
#define _NEWBITMAPDIALOG_HPP_

#include <QDialog>
#include <QString>
#include <epd_gfx/common.h>

#include "common/Common.h"

class QComboBox;
class QLabel;
class QLineEdit;
class QSlider;

LEKCO_BEGIN_NAMESPACE

class NewBitmapDialog
    : public QDialog
{
    Q_OBJECT

public:
    explicit NewBitmapDialog(QWidget* parent = nullptr);

    QString bitmapName() const;
    QString sourceImagePath() const;
    uint16_t targetWidth() const;
    uint16_t targetHeight() const;
    epd_gfx_format_t outputFormat() const;

private:
    void accept() override;
    void browseSourceImage();
    void refreshSize();

    QLineEdit* m_bitmapNameEdit  = nullptr;
    QLineEdit* m_sourceImageEdit = nullptr;
    QSlider*   m_scaleSlider     = nullptr;
    QComboBox* m_formatCombo     = nullptr;
    QLabel*    m_scaleLabel      = nullptr;
    QLabel*    m_sizeLabel       = nullptr;
    int        m_sourceWidth     = 0;
    int        m_sourceHeight    = 0;
};

LEKCO_END_NAMESPACE

#endif // !_NEWBITMAPDIALOG_HPP_
