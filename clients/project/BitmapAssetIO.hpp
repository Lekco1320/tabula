/**
 * @file BitmapAssetIO.hpp
 * @brief EBM bitmap asset file I/O helpers.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2026-05-20
 * @license MIT
 */

#pragma once

#ifndef _BITMAPASSETIO_HPP_
#define _BITMAPASSETIO_HPP_

#include <QString>
#include <QImage>
#include <epd_core/common.h>
#include <epd_gfx/ebm.h>
#include <epd_gfx/frame_view.h>

#include "common/Common.h"

LEKCO_BEGIN_NAMESPACE

struct ProjectBitmapInfo;

namespace BitmapAssetIO {

QString errorText(epd_err_t err);

bool readHeader(const QString& path, epd_gfx_ebm_header_t* outHeader);
bool isValidEbmFile(const QString& path);
bool frameViewToImage(const epd_gfx_frame_view_t* view, QImage* outImage);
bool loadEbmImage(const QString& path, QImage* outImage);
bool generateFrame(const QString& sourcePath, const ProjectBitmapInfo& info,
    epd_gfx_frame_view_t* outView, QString* error = nullptr);
bool saveFrameView(const QString& path, const epd_gfx_frame_view_t* view,
    QString* error = nullptr);
bool generateAndSave(const QString& sourcePath, const QString& targetPath,
    const ProjectBitmapInfo& info, QString* error = nullptr);

}

LEKCO_END_NAMESPACE

#endif // !_BITMAPASSETIO_HPP_
