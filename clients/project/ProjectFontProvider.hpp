/**
 * @file ProjectFontProvider.hpp
 * @brief Project-backed font resource provider.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2026-05-16
 * @license MIT
 */

#pragma once

#ifndef _PROJECTFONTPROVIDER_HPP_
#define _PROJECTFONTPROVIDER_HPP_

#include "project/FontProvider.hpp"
#include "project/Project.hpp"

LEKCO_BEGIN_NAMESPACE

class ProjectFontProvider
    : public FontProvider
{
public:
    explicit ProjectFontProvider(const Project* project = nullptr);

    void setProject(const Project* project);

    QVector<FontResourceInfo> fonts() const override;
    QVector<uint16_t> sizes(const QString& fileName) const override;
    bool font(const QString& fileName, FontResourceInfo* outInfo) const override;

private:
    const Project* m_project = nullptr;
};

LEKCO_END_NAMESPACE

#endif // !_PROJECTFONTPROVIDER_HPP_
