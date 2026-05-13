/**
 * @file ResourceWorkspace.hpp
 * @brief Base resource workspace for the tabula desktop client.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2026-05-13
 * @license MIT
 */

#pragma once

#ifndef _RESOURCEWORKSPACE_HPP_
#define _RESOURCEWORKSPACE_HPP_

#include <QWidget>

#include "common/Common.h"
#include "project/Project.hpp"

LEKCO_BEGIN_NAMESPACE

class ResourceWorkspace
    : public QWidget
{
    Q_OBJECT

public:
    explicit ResourceWorkspace(QWidget* parent = nullptr);

    virtual void setResource(const ProjectResource& resource) = 0;
    virtual void clearResource() = 0;
};

LEKCO_END_NAMESPACE

#endif // !_RESOURCEWORKSPACE_HPP_
