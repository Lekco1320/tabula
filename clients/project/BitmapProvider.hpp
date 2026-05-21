/**
 * @file BitmapProvider.hpp
 * @brief Project-backed bitmap resource provider for canvas tools.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2026-05-21
 * @license MIT
 */

#pragma once

#ifndef _BITMAPPROVIDER_HPP_
#define _BITMAPPROVIDER_HPP_

#include <stdint.h>
#include <QString>
#include <QVector>
#include <epd_gfx/canvas.h>

#include "common/Common.h"

LEKCO_BEGIN_NAMESPACE

class Project;

struct BitmapResourceInfo {
    QString          fileName;
    QString          displayName;
    QString          absolutePath;
    uint16_t         width  = 0U;
    uint16_t         height = 0U;
    epd_gfx_format_t format = EPD_GFX_FORMAT_NATIVE;
};

struct BitmapDrawRequest {
    QString         fileName;
    epd_gfx_point_t point;
};

class BitmapProvider
{
public:
    explicit BitmapProvider(const Project& project);

    QVector<BitmapResourceInfo> bitmaps() const;
    bool bitmap(const QString& fileName, BitmapResourceInfo* outInfo) const;
    bool hasUsableBitmaps() const;
    epd_err_t drawBitmap(epd_gfx_canvas_t canvas, const BitmapDrawRequest& request) const;

private:
    const Project& m_project;
};

LEKCO_END_NAMESPACE

#endif // !_BITMAPPROVIDER_HPP_
