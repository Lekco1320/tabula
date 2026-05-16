/**
 * @file ToolBar.cpp
 * @brief Exclusive tool selector.
 * 
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2025-12-16
 * @license MIT
 */

#include <QIcon>
#include <QToolButton>
#include <QButtonGroup>

#include "controls/bars/ToolBar.hpp"
#include "controls/widgets/FlowLayout.hpp"

LEKCO_BEGIN_NAMESPACE

#define ADD_TOOLBUTTON(_CLASS_, _DESCRIPTION_) \
  m_layout->addWidget(appendButton(static_cast<int>(Tool::_CLASS_), \
    QIcon(QStringLiteral(":/common/icons/" #_CLASS_ ".svg")), \
    QStringLiteral(_DESCRIPTION_)))

ToolBar::ToolBar(QWidget* parent)
    : IconButtonBar(QStringLiteral("Tools"), true, parent)
    , m_layout(new FlowLayout(this, 5, 2, 2))
    , m_current(Tool::None)
{
    ADD_TOOLBUTTON(DrawHLine, "Draw horizontal line");
    ADD_TOOLBUTTON(DrawVLine, "Draw vertical line");
    ADD_TOOLBUTTON(DrawRect,  "Draw rectangle");
    ADD_TOOLBUTTON(FillRect,  "Fill rectangle");
    ADD_TOOLBUTTON(DrawPixel, "Draw pixel");
    ADD_TOOLBUTTON(FillPanel, "Fill panel");
    ADD_TOOLBUTTON(DrawText,  "Draw text");

    connect(static_cast<IconButtonBar*>(this), &IconButtonBar::selectionChanged, this, [this](int id) {
        m_current = static_cast<Tool>(id);
        emit toolChanged(m_current);
    });
}

ToolBar::Tool ToolBar::currentTool() const
{
    return m_current;
}

LEKCO_END_NAMESPACE
