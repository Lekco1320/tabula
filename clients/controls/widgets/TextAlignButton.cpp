/**
 * @file TextAlignButton.cpp
 * @brief Cycle button for text alignment.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2026-05-18
 * @license MIT
 */

#include <QString>

#include "controls/widgets/TextAlignButton.hpp"

LEKCO_BEGIN_NAMESPACE

TextAlignButton::TextAlignButton(QWidget* parent)
    : CycleIconButton(parent)
{
    setToolTip(QStringLiteral("Text align"));
    setIcons({
        QStringLiteral(":/common/icons/TextAlignLeft.svg"),
        QStringLiteral(":/common/icons/TextAlignCenter.svg"),
        QStringLiteral(":/common/icons/TextAlignRight.svg"),
    });
}

epd_gfx_text_align_t TextAlignButton::currentAlign() const
{
    switch (currentIndex()) {
    case 1:
        return EPD_GFX_TEXT_ALIGN_CENTER;

    case 2:
        return EPD_GFX_TEXT_ALIGN_END;

    default:
        return EPD_GFX_TEXT_ALIGN_START;
    }
}

LEKCO_END_NAMESPACE
