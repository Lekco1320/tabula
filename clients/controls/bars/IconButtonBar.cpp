/**
 * @file IconButtonBar.cpp
 * @brief A bar containing buttons.
 * 
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2025-12-17
 * @license MIT
 */

#include <QToolButton>
#include <QButtonGroup>
#include <QSizePolicy>

#include "controls/bars/IconButtonBar.hpp"

LEKCO_BEGIN_NAMESPACE

IconButtonBar::IconButtonBar(const QString& title, bool optional, QWidget* parent)
    : QGroupBox(title, parent)
    , m_optional(optional)
    , m_btnGroup(new QButtonGroup(this))
{
    m_btnGroup->setExclusive(!m_optional);
    if (!m_optional) {
        connect(m_btnGroup, &QButtonGroup::idClicked, this, [this](int id) {
            emit selectionChanged(id);
        });
    }
}

QToolButton* IconButtonBar::appendButton(int id, const QIcon& icon, const QString& tooltip)
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

    if (!m_optional && m_btnGroup->buttons().isEmpty()) {
        btn->setChecked(true);
    }
    m_btnGroup->addButton(btn, id);

    if (m_optional) {
        btn->setAutoExclusive(false);
        connect(btn, &QAbstractButton::toggled, this, [this, btn, id](bool on) {
            if (on) {
                for (auto* other : m_btnGroup->buttons()) {
                    if (other != btn) {
                        other->setChecked(false);
                    }
                }
                emit selectionChanged(id);
            } else {
                bool anyChecked = false;
                for (auto* other : m_btnGroup->buttons()) {
                    if (other->isChecked()) {
                        anyChecked = true;
                        break;
                    }
                }
                if (!anyChecked) {
                    emit selectionChanged(-1);
                }
            }
        });
    }
    return btn;
}

LEKCO_END_NAMESPACE
