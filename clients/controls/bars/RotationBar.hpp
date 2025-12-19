/**
 * @file RotationBar.hpp
 * @brief Exclusive rotation selector.
 * 
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2025-12-19
 * @license MIT
 */

#pragma once

#ifndef _ROTATIONBAR_HPP_
#define _ROTATIONBAR_HPP_

#include <epd_gfx/common.h>

#include "common/common.h"
#include "controls/bars/IconButtonBar.hpp"

LEKCO_BEGIN_NAMESPACE

class FlowLayout;

class RotationBar
    : public IconButtonBar
{
    Q_OBJECT

public:
    using Rotation = epd_gfx_rotation_t;
    explicit RotationBar(QWidget* parent = nullptr);

signals:
    void rotationChanged(Rotation rotation);

protected:
    FlowLayout* m_layout;
    Rotation    m_rotation;
};

LEKCO_END_NAMESPACE

#endif // !_ROTATIONBAR_HPP_
