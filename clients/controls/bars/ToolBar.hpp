/**
 * @file ToolBar.hpp
 * @brief Exclusive tool selector.
 * 
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2025-12-16
 * @license MIT
 */

#pragma once

#ifndef _TOOLBAR_HPP_
#define _TOOLBAR_HPP_

#include <QGroupBox>

#include "common/Common.h"
#include "controls/bars/IconButtonBar.hpp"

class QIcon;
class QButtonGroup;
class QToolButton;

LEKCO_BEGIN_NAMESPACE

class FlowLayout;

class ToolBar
    : public IconButtonBar
{
    Q_OBJECT

public:
    enum class Tool {
        DrawHLine, DrawVLine, DrawRect,
        FillRect, DrawPixel, FillPanel,
        DrawText, None
    };

    explicit ToolBar(QWidget* parent = nullptr);
    
    Tool currentTool() const;
    void setToolEnabled(Tool tool, bool enabled);

signals:
    void toolChanged(Tool tool);

private:
    FlowLayout* m_layout;
    Tool        m_current;
};

LEKCO_END_NAMESPACE

#endif // !_TOOLBAR_HPP_
