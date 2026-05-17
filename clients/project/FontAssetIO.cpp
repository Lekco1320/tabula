/**
 * @file FontAssetIO.cpp
 * @brief EGF font asset file I/O helper implementation.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2026-05-17
 * @license MIT
 */

#include <QFile>
#include <QSaveFile>

#include "project/EpdStreamAdapter.hpp"
#include "project/FontAssetIO.hpp"

LEKCO_BEGIN_NAMESPACE

BEGIN_NAMESPACE()

bool ReadHeaderFromFile(QFile* file, epd_gfx_egf_header_t* outHeader)
{
    EpdStreamAdapter stream(file);
    epd_gfx_egf_header_t header = {};
    if (!epd_gfx_egf_read_header(stream.stream(), &header) || !epd_gfx_egf_check_magic(&header)) {
        return false;
    }

    *outHeader = header;
    return true;
}

END_NAMESPACE

namespace FontAssetIO {

QString errorText(epd_err_t err)
{
    return QString::fromLatin1(epd_err_to_str(err));
}

bool readHeader(const QString& path, epd_gfx_egf_header_t* outHeader)
{
    if (!outHeader) {
        return false;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    epd_gfx_egf_header_t header = {};
    if (!ReadHeaderFromFile(&file, &header)) {
        return false;
    }

    *outHeader = header;
    return true;
}

bool isValidEgfFile(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    epd_gfx_egf_header_t header = {};
    if (!ReadHeaderFromFile(&file, &header)) {
        return false;
    }

    const quint64 expectedSize = EPD_GFX_EGF_HEADER_BYTES
        + static_cast<quint64>(header.size_count) * EPD_GFX_EGF_SIZE_RECORD_BYTES
        + static_cast<quint64>(header.glyph_count) * EPD_GFX_EGF_GLYPH_INDEX_BYTES
        + header.data_count;
    return static_cast<quint64>(file.size()) == expectedSize;
}

QVector<uint16_t> readSizes(const QString& path)
{
    QVector<uint16_t> result;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return result;
    }

    epd_gfx_egf_header_t header = {};
    if (!ReadHeaderFromFile(&file, &header)) {
        return result;
    }

    EpdStreamAdapter stream(&file);
    for (uint32_t i = 0U; i < header.size_count; ++i) {
        epd_gfx_egf_size_record_t record = {};
        if (epd_gfx_egf_read_size_record_at(stream.stream(), i, &record) != EPD_OK) {
            return QVector<uint16_t>();
        }
        if (record.size != 0U) {
            result.append(record.size);
        }
    }
    return result;
}

epd_err_t withRuntimeFont(const QString& path, const std::function<epd_err_t(epd_gfx_font_t)>& callback)
{
    if (!callback) {
        return EPD_ERR_INVALID_ARG;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return EPD_ERR_INVALID_STATE;
    }

    EpdStreamAdapter stream(&file);
    epd_gfx_font_t font = nullptr;
    epd_err_t      ret  = epd_gfx_font_load(stream.stream(), &font);
    if (ret != EPD_OK) {
        return ret;
    }

    ret = callback(font);
    epd_gfx_font_destroy(font);
    return ret;
}

epd_err_t loadEditableAsset(const QString& path, epd_asset_font_asset_t* outAsset)
{
    if (!outAsset) {
        return EPD_ERR_INVALID_ARG;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return EPD_ERR_INVALID_STATE;
    }

    EpdStreamAdapter stream(&file);
    return epd_asset_font_asset_load_egf(stream.stream(), outAsset);
}

bool saveEditableAsset(const QString& path, epd_asset_font_asset_t asset, QString* error)
{
    if (!asset || path.isEmpty()) {
        if (error) {
            *error = QStringLiteral("Invalid font asset.");
        }
        return false;
    }

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        if (error) {
            *error = QStringLiteral("Failed to open font file for writing.");
        }
        return false;
    }

    EpdStreamAdapter stream(&file);
    const epd_err_t ret = epd_asset_font_asset_write_egf(asset, stream.stream());
    if (ret != EPD_OK) {
        if (error) {
            *error = QStringLiteral("Failed to save font file: %1").arg(errorText(ret));
        }
        return false;
    }

    if (!file.commit()) {
        if (error) {
            *error = QStringLiteral("Failed to save font file: commit failed");
        }
        return false;
    }
    return true;
}

}

LEKCO_END_NAMESPACE
