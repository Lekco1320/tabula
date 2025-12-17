/**
 * @file IconButtonBar.hpp
 * @brief A bar containing buttons.
 * 
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2025-12-17
 * @license MIT
 */

#pragma once

#ifndef _BUTTONBAR_HPP_
#define _BUTTONBAR_HPP_

#include <QString>
#include <QGroupBox>

#include "common.h"

class QButtonGroup;
class QToolButton;
class QIcon;

LEKCO_BEGIN_NAMESPACE

class IconButtonBar
    : public QGroupBox
{
    Q_OBJECT

public:
    explicit IconButtonBar(const QString& title, bool canCancel, QWidget* parent = nullptr);

signals:
    void selectionChanged(int id);

protected:
    bool          m_optional;
    QButtonGroup* m_btnGroup;

    QToolButton* appendButton(int id, const QIcon& icon, const QString& tooltip);
};

LEKCO_END_NAMESPACE

#endif // !_BUTTONBAR_HPP_
