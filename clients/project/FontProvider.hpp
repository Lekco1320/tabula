/**
 * @file FontProvider.hpp
 * @brief Project-backed font resource provider for canvas tools.
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
#include <epd_gfx/text.h>

#include "common/Common.h"

LEKCO_BEGIN_NAMESPACE

class Project;

struct FontResourceInfo {
    QString fileName;
    QString displayName;
    QString absolutePath;
};

struct FontTextDrawRequest {
    QString              fileName;
    QString              text;
    epd_gfx_point_t      origin;
    epd_gfx_text_style_t style;
};

class FontProvider
{
public:
    explicit FontProvider(const Project& project);

    QVector<FontResourceInfo> fonts() const;
    QVector<uint16_t> sizes(const QString& fileName) const;
    bool font(const QString& fileName, FontResourceInfo* outInfo) const;
    bool hasUsableFonts() const;
    bool hasRenderableText(const QString& fileName, uint16_t size, const QString& text) const;
    epd_err_t drawText(epd_gfx_canvas_t canvas, const FontTextDrawRequest& request) const;

private:
    const Project& m_project;
};

LEKCO_END_NAMESPACE

#endif // !_FONTPROVIDER_HPP_
