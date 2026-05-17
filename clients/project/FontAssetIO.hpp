/**
 * @file FontAssetIO.hpp
 * @brief EGF font asset file I/O helpers.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2026-05-17
 * @license MIT
 */

#pragma once

#ifndef _FONTASSETIO_HPP_
#define _FONTASSETIO_HPP_

#include <functional>
#include <stdint.h>
#include <QString>
#include <QVector>
#include <epd_asset/font_asset.h>
#include <epd_core/common.h>
#include <epd_gfx/egf.h>
#include <epd_gfx/font.h>

#include "common/Common.h"

LEKCO_BEGIN_NAMESPACE

namespace FontAssetIO {

QString errorText(epd_err_t err);

bool readHeader(const QString& path, epd_gfx_egf_header_t* outHeader);
bool isValidEgfFile(const QString& path);
QVector<uint16_t> readSizes(const QString& path);

epd_err_t withRuntimeFont(const QString& path, const std::function<epd_err_t(epd_gfx_font_t)>& callback);
epd_err_t loadEditableAsset(const QString& path, epd_asset_font_asset_t* outAsset);
bool saveEditableAsset(const QString& path, epd_asset_font_asset_t asset, QString* error = nullptr);

}

LEKCO_END_NAMESPACE

#endif // !_FONTASSETIO_HPP_
