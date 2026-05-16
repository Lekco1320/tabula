/**
 * @file FlowButton.hpp
 * @brief Toggle button for text flow direction.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2026-05-16
 * @license MIT
 */

#pragma once

#ifndef _FLOWBUTTON_HPP_
#define _FLOWBUTTON_HPP_

#include <QToolButton>
#include <epd_gfx/text.h>

#include "common/Common.h"

LEKCO_BEGIN_NAMESPACE

class FlowButton
    : public QToolButton
{
    Q_OBJECT

public:
    explicit FlowButton(QWidget* parent = nullptr);

    epd_gfx_text_flow_t currentFlow() const;

signals:
    void flowChanged(epd_gfx_text_flow_t flow);

private:
    void updateIcon();
    void nextFlow();

    epd_gfx_text_flow_t m_flow = EPD_GFX_TEXT_FLOW_HORIZONTAL;
};

LEKCO_END_NAMESPACE

#endif // !_FLOWBUTTON_HPP_
