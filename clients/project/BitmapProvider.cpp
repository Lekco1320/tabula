/**
 * @file BitmapProvider.cpp
 * @brief Project-backed bitmap resource provider implementation.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2026-05-21
 * @license MIT
 */

#include <QFile>
#include <QFileInfo>
#include <epd_gfx/bitmap.h>

#include "project/BitmapAssetIO.hpp"
#include "project/BitmapProvider.hpp"
#include "project/EpdStreamAdapter.hpp"
#include "project/Project.hpp"

LEKCO_BEGIN_NAMESPACE

BitmapProvider::BitmapProvider(const Project& project)
    : m_project(project)
{
}

QVector<BitmapResourceInfo> BitmapProvider::bitmaps() const
{
    QVector<BitmapResourceInfo> result;

    const QVector<ProjectResource> resources = m_project.resources(ProjectResourceType::Bitmaps);
    for (const ProjectResource& resource : resources) {
        epd_gfx_ebm_header_t header = {};
        if (!BitmapAssetIO::readHeader(resource.absolutePath, &header)) {
            continue;
        }

        BitmapResourceInfo info;
        info.fileName     = resource.fileName;
        info.displayName  = QFileInfo(resource.fileName).completeBaseName();
        info.absolutePath = resource.absolutePath;
        info.width        = header.width;
        info.height       = header.height;
        info.format       = header.format;
        result.append(info);
    }
    return result;
}

bool BitmapProvider::bitmap(const QString& fileName, BitmapResourceInfo* outInfo) const
{
    if (fileName.isEmpty()) {
        return false;
    }

    const QVector<BitmapResourceInfo> bitmapList = bitmaps();
    for (const BitmapResourceInfo& info : bitmapList) {
        if (info.fileName == fileName) {
            if (outInfo) {
                *outInfo = info;
            }
            return true;
        }
    }
    return false;
}

bool BitmapProvider::hasUsableBitmaps() const
{
    return !bitmaps().isEmpty();
}

epd_err_t BitmapProvider::drawBitmap(epd_gfx_canvas_t canvas, const BitmapDrawRequest& request) const
{
    if (!canvas || request.fileName.isEmpty()) {
        return EPD_ERR_INVALID_ARG;
    }

    BitmapResourceInfo info;
    if (!bitmap(request.fileName, &info)) {
        return EPD_ERR_NOT_FOUND;
    }

    QFile file(info.absolutePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return EPD_ERR_INVALID_STATE;
    }

    EpdStreamAdapter stream(&file);
    epd_gfx_bitmap_t bitmap = nullptr;
    epd_err_t        ret    = epd_gfx_bitmap_load(stream.stream(), &bitmap);
    if (ret != EPD_OK) {
        return ret;
    }

    ret = epd_gfx_canvas_draw_bitmap(canvas, bitmap, request.point);
    const epd_err_t destroyRet = epd_gfx_bitmap_destroy(bitmap);
    return ret == EPD_OK ? destroyRet : ret;
}

LEKCO_END_NAMESPACE
