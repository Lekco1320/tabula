/**
 * @file NewFontDialog.hpp
 * @brief New EGF font dialog for project assets.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2026-05-13
 * @license MIT
 */

#pragma once

#ifndef _NEWFONTDIALOG_HPP_
#define _NEWFONTDIALOG_HPP_

#include <QDialog>
#include <QString>

#include "common/Common.h"

class QLineEdit;

LEKCO_BEGIN_NAMESPACE

class NewFontDialog
    : public QDialog
{
    Q_OBJECT

public:
    explicit NewFontDialog(QWidget* parent = nullptr);

    QString fontName() const;
    QString sourceFontPath() const;

private:
    void accept() override;
    void browseSourceFont();

    QLineEdit* m_fontNameEdit   = nullptr;
    QLineEdit* m_sourceFontEdit = nullptr;
};

LEKCO_END_NAMESPACE

#endif // !_NEWFONTDIALOG_HPP_
