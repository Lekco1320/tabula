/**
 * @file FontProvider.hpp
 * @brief Font resource provider interface for canvas tools.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2026-05-16
 * @license MIT
 */

#pragma once

#ifndef _FONTPROVIDER_HPP_
#define _FONTPROVIDER_HPP_

#include <stdint.h>
#include <QString>
#include <QVector>

#include "common/Common.h"

LEKCO_BEGIN_NAMESPACE

struct FontResourceInfo {
    QString fileName;
    QString displayName;
    QString absolutePath;
};

class FontProvider
{
public:
    virtual ~FontProvider() = default;

    virtual QVector<FontResourceInfo> fonts() const = 0;
    virtual QVector<uint16_t> sizes(const QString& fileName) const = 0;
    virtual bool font(const QString& fileName, FontResourceInfo* outInfo) const = 0;
};

LEKCO_END_NAMESPACE

#endif // !_FONTPROVIDER_HPP_
