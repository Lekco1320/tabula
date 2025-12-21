/**
 * @file SetupDialog.hpp
 * @brief Dialog for setting up screen parameters.
 * 
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2025-12-21
 * @license MIT
 */

#pragma once

#ifndef _SETUPDIALOG_H_
#define _SETUPDIALOG_H_

#include <QDialog>
#include <epd_gfx/canvas.h>

#include "common/Common.h"

class QSpinBox;
class QComboBox;

LEKCO_BEGIN_NAMESPACE

class SetupDialog
    : public QDialog
{
    Q_OBJECT

public:
    explicit SetupDialog(QWidget *parent = nullptr);

    int panelWidth() const;
    int panelHeight() const;
    epd_gfx_format_t format() const;
    epd_gfx_rotation_t rotation() const;

private:
    QSpinBox*  m_widthSpinBox;
    QSpinBox*  m_heightSpinBox;
    QComboBox* m_formatComboBox;
    QComboBox* m_rotationComboBox;
};

LEKCO_END_NAMESPACE

#endif // !_SETUPDIALOG_H_
