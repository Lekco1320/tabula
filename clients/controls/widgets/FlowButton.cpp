/**
 * @file FlowButton.cpp
 * @brief Toggle button for text flow direction.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2026-05-16
 * @license MIT
 */

#include <QIcon>
#include <QSizePolicy>

#include "controls/widgets/FlowButton.hpp"

LEKCO_BEGIN_NAMESPACE

FlowButton::FlowButton(QWidget* parent)
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

    updateIcon();
    connect(this, &QToolButton::clicked, this, &FlowButton::nextFlow);
}

epd_gfx_text_flow_t FlowButton::currentFlow() const
{
    return m_flow;
}

void FlowButton::updateIcon()
{
    const QString path = (m_flow == EPD_GFX_TEXT_FLOW_VERTICAL)
        ? QStringLiteral(":/common/icons/TextVAlign.svg")
        : QStringLiteral(":/common/icons/TextHAlign.svg");
    setIcon(QIcon(path));
    setIconSize(QSize(20, 20));
}

void FlowButton::nextFlow()
{
    m_flow = (m_flow == EPD_GFX_TEXT_FLOW_VERTICAL)
        ? EPD_GFX_TEXT_FLOW_HORIZONTAL
        : EPD_GFX_TEXT_FLOW_VERTICAL;
    updateIcon();
    emit flowChanged(m_flow);
}

LEKCO_END_NAMESPACE
