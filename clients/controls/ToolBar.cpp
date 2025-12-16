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
#include <QSizePolicy>

#include "ToolBar.hpp"
#include "FlowLayout.hpp"

LEKCO_BEGIN_NAMESPACE

#define ADD_TOOLBUTTON(_CLASS_, _DESCRIPTION_) \
  addButton(Tool::_CLASS_, \
    QIcon(QStringLiteral(":/common/icons/" #_CLASS_ ".svg")), \
    QStringLiteral(_DESCRIPTION_))

ToolBar::ToolBar(QWidget* parent)
    : QGroupBox(parent)
    , m_group(new QButtonGroup(this))
    , m_layout(new FlowLayout(this, 5, 2, 2))
    , m_current(Tool::Pointer)
{
    m_group->setExclusive(true);
    setLayout(m_layout);
    setContentsMargins(0, 0, 0, 0);

    ADD_TOOLBUTTON(Pointer,   "Pointer");
    ADD_TOOLBUTTON(Inspect,   "Inspect");
    ADD_TOOLBUTTON(DrawHLine, "Draw horizontal line");
    ADD_TOOLBUTTON(DrawVLine, "Draw vertical line");
    ADD_TOOLBUTTON(DrawRect,  "Draw rectangle");
    ADD_TOOLBUTTON(FillRect,  "Fill rectangle");
    ADD_TOOLBUTTON(DrawPixel, "Draw pixel");
    ADD_TOOLBUTTON(FillPanel, "Fill panel");

    connect(m_group, &QButtonGroup::idClicked, this, [this](int id) {
        m_current = static_cast<Tool>(id);
        emit toolChanged(m_current);
    });
}

ToolBar::Tool ToolBar::currentTool() const
{
    return m_current;
}

QToolButton* ToolBar::addButton(Tool tool, const QIcon& icon, const QString& tooltip)
{
    auto* btn = new QToolButton(this);
    btn->setIcon(icon);
    btn->setToolTip(tooltip);
    btn->setCheckable(true);
    btn->setAutoExclusive(true);
    btn->setAutoRaise(true);
    btn->setToolButtonStyle(Qt::ToolButtonIconOnly);
    btn->setFocusPolicy(Qt::NoFocus);
    btn->setIconSize(QSize(22, 22));
    btn->setFixedSize(QSize(28, 28));
    btn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    if (m_group->buttons().isEmpty()) {
        btn->setChecked(true);
    }
    m_group->addButton(btn, static_cast<int>(tool));
    m_layout->addWidget(btn);
    return btn;
}

LEKCO_END_NAMESPACE
