/**
 * @file CycleIconButton.hpp
 * @brief Tool button for cycling through icon states.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2026-05-18
 * @license MIT
 */

#pragma once

#ifndef _CYCLEICONBUTTON_HPP_
#define _CYCLEICONBUTTON_HPP_

#include <QStringList>
#include <QToolButton>

#include "common/Common.h"

LEKCO_BEGIN_NAMESPACE

class CycleIconButton
    : public QToolButton
{
    Q_OBJECT

public:
    explicit CycleIconButton(QWidget* parent = nullptr);

    int currentIndex() const;
    void setCurrentIndex(int index);

signals:
    void currentIndexChanged(int index);

protected:
    void setIcons(const QStringList& icons);

private:
    void nextIndex();
    void updateIcon();

    QStringList m_icons;
    int         m_currentIndex = 0;
};

LEKCO_END_NAMESPACE

#endif // !_CYCLEICONBUTTON_HPP_
