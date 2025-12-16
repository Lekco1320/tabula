/**
 * @file ToolBar.hpp
 * @brief Exclusive tool selector.
 * 
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2025-12-16
 * @license MIT
 */

#pragma once

#include <QGroupBox>

#include "common.h"
#include "FlowLayout.hpp"

class QIcon;
class QButtonGroup;
class QToolButton;

LEKCO_BEGIN_NAMESPACE

class ToolBar
    : public QGroupBox
{
    Q_OBJECT

public:
    enum class Tool {
        Pointer, Inspect, DrawHLine, DrawVLine,
        DrawRect, FillRect, DrawPixel, FillPanel,
    };

    explicit ToolBar(QWidget* parent = nullptr);

    Tool currentTool() const;

signals:
    void toolChanged(Tool tool);

private:
    QToolButton* addButton(Tool tool, const QIcon& icon, const QString& tooltip);
    QButtonGroup* m_group;
    FlowLayout*   m_layout;
    Tool          m_current;
};

LEKCO_END_NAMESPACE
