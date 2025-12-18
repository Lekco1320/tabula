/**
 * @file CursorBar.hpp
 * @brief Exclusive cursor selector.
 * 
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2025-12-17
 * @license MIT
 */

#pragma once

#ifndef _CURSORBAR_HPP_
#define _CURSORBAR_HPP_

#include "common/common.h"
#include "controls/bars/IconButtonBar.hpp"
#include "controls/widgets/CanvasPreviewer.hpp"

LEKCO_BEGIN_NAMESPACE

class FlowLayout;

class CursorBar
    : public IconButtonBar
{
    Q_OBJECT

public:
    using Cursor = CanvasPreviewer::Cursor;
    explicit CursorBar(QWidget* parent = nullptr);

signals:
    void cursorChanged(Cursor cursor);

protected:
    FlowLayout* m_layout;
    Cursor      m_cursor;
};

LEKCO_END_NAMESPACE

#endif // !_CURSORBAR_HPP_
