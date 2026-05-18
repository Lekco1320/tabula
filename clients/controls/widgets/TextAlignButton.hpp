/**
 * @file TextAlignButton.hpp
 * @brief Cycle button for text alignment.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2026-05-18
 * @license MIT
 */

#pragma once

#ifndef _TEXTALIGNBUTTON_HPP_
#define _TEXTALIGNBUTTON_HPP_

#include <epd_gfx/text.h>

#include "controls/widgets/CycleIconButton.hpp"

LEKCO_BEGIN_NAMESPACE

class TextAlignButton
    : public CycleIconButton
{
    Q_OBJECT

public:
    explicit TextAlignButton(QWidget* parent = nullptr);

    epd_gfx_text_align_t currentAlign() const;
};

LEKCO_END_NAMESPACE

#endif // !_TEXTALIGNBUTTON_HPP_
