/**
 * @file TextWrapButton.cpp
 * @brief Cycle button for text box wrapping.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2026-05-18
 * @license MIT
 */

#include <QString>

#include "controls/widgets/TextWrapButton.hpp"

LEKCO_BEGIN_NAMESPACE

TextWrapButton::TextWrapButton(QWidget* parent)
    : CycleIconButton(parent)
{
    setToolTip(QStringLiteral("Text wrap"));
    setIcons({
        QStringLiteral(":/common/icons/TextWrapOverflow.svg"),
        QStringLiteral(":/common/icons/TextWrapTruncation.svg"),
    });
}

bool TextWrapButton::wrap() const
{
    return currentIndex() == 1;
}

LEKCO_END_NAMESPACE
