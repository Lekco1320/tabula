/**
 * @file TextWrapButton.hpp
 * @brief Cycle button for text box wrapping.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2026-05-18
 * @license MIT
 */

#pragma once

#ifndef _TEXTWRAPBUTTON_HPP_
#define _TEXTWRAPBUTTON_HPP_

#include "controls/widgets/CycleIconButton.hpp"

LEKCO_BEGIN_NAMESPACE

class TextWrapButton
    : public CycleIconButton
{
    Q_OBJECT

public:
    explicit TextWrapButton(QWidget* parent = nullptr);

    bool wrap() const;
};

LEKCO_END_NAMESPACE

#endif // !_TEXTWRAPBUTTON_HPP_
