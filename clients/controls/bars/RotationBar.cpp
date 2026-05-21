/**
 * @file RotationBar.cpp
 * @brief Exclusive rotation selector.
 * 
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2025-12-19
 * @license MIT
 */

#include <QIcon>
#include <QToolButton>
#include <QButtonGroup>

#include "controls/bars/RotationBar.hpp"
#include "controls/widgets/FlowLayout.hpp"

LEKCO_BEGIN_NAMESPACE

#define ADD_TOOLBUTTON(_ROT_, _ICON_, _DESCRIPTION_) \
  m_layout->addWidget(appendButton(static_cast<int>(_ROT_), \
    QIcon(QStringLiteral(":/common/icons/" #_ICON_ ".svg")), \
    QStringLiteral(_DESCRIPTION_)))

RotationBar::RotationBar(QWidget* parent)
    : IconButtonBar(QStringLiteral("Rotation"), false, parent)
    , m_layout(new FlowLayout(this, 3, 2, 2))
    , m_rotation(EPD_GFX_ROTATE_0)
{
    ADD_TOOLBUTTON(EPD_GFX_ROTATE_0, Rotation0, "Rotate 0°");
    ADD_TOOLBUTTON(EPD_GFX_ROTATE_90, Rotation90, "Rotate 90°");
    ADD_TOOLBUTTON(EPD_GFX_ROTATE_180, Rotation180, "Rotate 180°");
    ADD_TOOLBUTTON(EPD_GFX_ROTATE_270, Rotation270, "Rotate 270°");

    connect(static_cast<IconButtonBar*>(this), &IconButtonBar::selectionChanged, this, [this](int id) {
        m_rotation = static_cast<Rotation>(id);
        emit rotationChanged(m_rotation);
    });
}

LEKCO_END_NAMESPACE
