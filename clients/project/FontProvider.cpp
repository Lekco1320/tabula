/**
 * @file FontProvider.cpp
 * @brief Project-backed font resource provider implementation.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2026-05-16
 * @license MIT
 */

#include <QByteArray>
#include <QFileInfo>
#include <epd_gfx/glyph.h>
#include <epd_gfx/text.h>

#include "project/FontAssetIO.hpp"
#include "project/FontProvider.hpp"
#include "project/Project.hpp"

LEKCO_BEGIN_NAMESPACE

FontProvider::FontProvider(const Project& project)
    : m_project(project)
{
}

QVector<FontResourceInfo> FontProvider::fonts() const
{
    QVector<FontResourceInfo> result;

    const QVector<ProjectResource> resources = m_project.resources(ProjectResourceType::Fonts);
    for (const ProjectResource& resource : resources) {
        FontResourceInfo info;
        info.fileName     = resource.fileName;
        info.displayName  = QFileInfo(resource.fileName).completeBaseName();
        info.absolutePath = resource.absolutePath;
        result.append(info);
    }
    return result;
}

QVector<uint16_t> FontProvider::sizes(const QString& fileName) const
{
    FontResourceInfo info;
    if (!font(fileName, &info)) {
        return QVector<uint16_t>();
    }
    return FontAssetIO::readSizes(info.absolutePath);
}

bool FontProvider::font(const QString& fileName, FontResourceInfo* outInfo) const
{
    if (fileName.isEmpty()) {
        return false;
    }

    const QVector<FontResourceInfo> fontList = fonts();
    for (const FontResourceInfo& info : fontList) {
        if (info.fileName == fileName) {
            if (outInfo) {
                *outInfo = info;
            }
            return true;
        }
    }
    return false;
}

bool FontProvider::hasUsableFonts() const
{
    const QVector<FontResourceInfo> fontList = fonts();
    for (const FontResourceInfo& info : fontList) {
        if (!sizes(info.fileName).isEmpty()) {
            return true;
        }
    }
    return false;
}

bool FontProvider::hasRenderableText(const QString& fileName, uint16_t size, const QString& text) const
{
    if (fileName.isEmpty() || size == 0U) {
        return false;
    }
    if (text.isEmpty()) {
        return true;
    }

    FontResourceInfo info;
    if (!font(fileName, &info)) {
        return false;
    }

    const QVector<uint> codepoints = text.toUcs4();
    const epd_err_t     ret        = FontAssetIO::withRuntimeFont(info.absolutePath, [size, codepoints](epd_gfx_font_t font) {
        for (uint codepoint : codepoints) {
            if (codepoint == '\r' || codepoint == '\n' || codepoint == '\t') {
                codepoint = ' ';
            }

            epd_gfx_glyph_key_t key;
            key.codepoint = static_cast<uint32_t>(codepoint);
            key.size      = size;
            if (!epd_gfx_font_contains_glyph(font, key)) {
                return EPD_ERR_NOT_FOUND;
            }
        }
        return EPD_OK;
    });
    return ret == EPD_OK;
}

epd_err_t FontProvider::drawText(epd_gfx_canvas_t canvas, const FontTextDrawRequest& request) const
{
    if (!canvas || request.fileName.isEmpty() || request.text.isEmpty() || request.style.size == 0U) {
        return EPD_ERR_INVALID_ARG;
    }

    FontResourceInfo info;
    if (!font(request.fileName, &info)) {
        return EPD_ERR_NOT_FOUND;
    }

    const QByteArray utf8 = request.text.toUtf8();
    return FontAssetIO::withRuntimeFont(info.absolutePath, [&request, utf8, canvas](epd_gfx_font_t font) {
        return epd_gfx_canvas_draw_utf8(canvas, font, utf8.constData(), request.origin, &request.style);
    });
}

LEKCO_END_NAMESPACE
