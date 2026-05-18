/**
 * @file CycleIconButton.cpp
 * @brief Tool button for cycling through icon states.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2026-05-18
 * @license MIT
 */

#include <QIcon>
#include <QSize>
#include <QSizePolicy>
#include <QString>

#include "controls/widgets/CycleIconButton.hpp"

LEKCO_BEGIN_NAMESPACE

CycleIconButton::CycleIconButton(QWidget* parent)
    : QToolButton(parent)
{
    setAutoRaise(false);
    setCheckable(false);
    setMinimumSize(24, 24);
    setMaximumSize(24, 24);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setStyleSheet(QStringLiteral(
        "QToolButton { border: 1px solid #a0a0a0; border-radius: 3px; padding: 0; }"
        "QToolButton:pressed { border: 1px solid #707070; }"));

    connect(this, &QToolButton::clicked, this, &CycleIconButton::nextIndex);
}

int CycleIconButton::currentIndex() const
{
    return m_currentIndex;
}

void CycleIconButton::setCurrentIndex(int index)
{
    if (index < 0 || index >= m_icons.size() || index == m_currentIndex) {
        return;
    }

    m_currentIndex = index;
    updateIcon();
    emit currentIndexChanged(m_currentIndex);
}

void CycleIconButton::setIcons(const QStringList& icons)
{
    m_icons        = icons;
    m_currentIndex = 0;
    updateIcon();
}

void CycleIconButton::nextIndex()
{
    if (m_icons.isEmpty()) {
        return;
    }

    m_currentIndex = (m_currentIndex + 1) % m_icons.size();
    updateIcon();
    emit currentIndexChanged(m_currentIndex);
}

void CycleIconButton::updateIcon()
{
    if (m_icons.isEmpty()) {
        setIcon(QIcon());
        return;
    }

    setIcon(QIcon(m_icons[m_currentIndex]));
    setIconSize(QSize(20, 20));
}

LEKCO_END_NAMESPACE
