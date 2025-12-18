/**
 * @file CursorBar.cpp
 * @brief Exclusive cursor selector.
 * 
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2025-12-17
 * @license MIT
 */

#include <QIcon>
#include <QToolButton>
#include <QButtonGroup>

#include "controls/bars/CursorBar.hpp"
#include "controls/widgets/FlowLayout.hpp"

LEKCO_BEGIN_NAMESPACE

#define ADD_TOOLBUTTON(_CLASS_, _DESCRIPTION_) \
  m_layout->addWidget(appendButton(static_cast<int>(Cursor::_CLASS_), \
    QIcon(QStringLiteral(":/common/icons/" #_CLASS_ ".svg")), \
    QStringLiteral(_DESCRIPTION_)))

CursorBar::CursorBar(QWidget* parent)
    : IconButtonBar(QStringLiteral("Cursor"), false, parent)
    , m_layout(new FlowLayout(this, 5, 2, 2))
    , m_cursor(Cursor::Pointer)
{
    ADD_TOOLBUTTON(Pointer, "Pointer");
    ADD_TOOLBUTTON(Inspect, "Inspect");

    connect(static_cast<IconButtonBar*>(this), &IconButtonBar::selectionChanged, this, [this](int id) {
        m_cursor = static_cast<Cursor>(id);
        emit cursorChanged(m_cursor);
    });
}

LEKCO_END_NAMESPACE
