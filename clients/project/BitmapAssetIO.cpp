/**
 * @file BitmapAssetIO.cpp
 * @brief EBM bitmap asset file I/O helper implementation.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2026-05-20
 * @license MIT
 */

#include <stdint.h>
#include <QFile>
#include <QImage>
#include <QSaveFile>
#include <epd_asset/bitmap_asset.h>
#include <epd_asset/bitmap_generator.h>
#include <epd_gfx/bitmap.h>

#include "controls/Utils.hpp"
#include "project/BitmapAssetIO.hpp"
#include "project/EpdStreamAdapter.hpp"
#include "project/Project.hpp"

LEKCO_BEGIN_NAMESPACE

BEGIN_NAMESPACE()

bool ReadHeaderFromFile(QFile* file, epd_gfx_ebm_header_t* outHeader)
{
    EpdStreamAdapter stream(file);
    epd_gfx_ebm_header_t header = {};
    if (!epd_gfx_ebm_read_header(stream.stream(), &header) || !epd_gfx_ebm_check_magic(&header)) {
        return false;
    }

    *outHeader = header;
    return true;
}

bool FillSourceFromImage(const QImage& image, QByteArray* outData, epd_asset_bitmap_source_t* outSource)
{
    if (image.isNull() || !outData || !outSource) {
        return false;
    }
    if (image.width() <= 0 || image.height() <= 0
        || image.width() > UINT16_MAX || image.height() > UINT16_MAX) {
        return false;
    }

    QImage rgb = image.convertToFormat(QImage::Format_RGB888);
    *outData = QByteArray(reinterpret_cast<const char*>(rgb.constBits()), rgb.sizeInBytes());

    outSource->data   = reinterpret_cast<const uint8_t*>(outData->constData());
    outSource->width  = static_cast<uint16_t>(rgb.width());
    outSource->height = static_cast<uint16_t>(rgb.height());
    outSource->stride = static_cast<uint32_t>(rgb.bytesPerLine());
    return true;
}

END_NAMESPACE

namespace BitmapAssetIO {

QString errorText(epd_err_t err)
{
    return QString::fromLatin1(epd_err_to_str(err));
}

bool readHeader(const QString& path, epd_gfx_ebm_header_t* outHeader)
{
    if (!outHeader) {
        return false;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    epd_gfx_ebm_header_t header = {};
    if (!ReadHeaderFromFile(&file, &header)) {
        return false;
    }

    *outHeader = header;
    return true;
}

bool isValidEbmFile(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    epd_gfx_ebm_header_t header = {};
    if (!ReadHeaderFromFile(&file, &header) || !epd_gfx_ebm_format_valid(header.format)) {
        return false;
    }
    if (header.width == 0U || header.height == 0U) {
        return false;
    }

    const quint64 expectedSize = EPD_GFX_EBM_HEADER_BYTES
        + epd_gfx_ebm_data_bytes(header.width, header.height, header.format);
    return static_cast<quint64>(file.size()) == expectedSize;
}

bool frameViewToImage(const epd_gfx_frame_view_t* view, QImage* outImage)
{
    return FrameViewToQImage(view, outImage);
}

bool loadEbmImage(const QString& path, QImage* outImage)
{
    if (!outImage) {
        return false;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    EpdStreamAdapter stream(&file);
    epd_gfx_bitmap_t bitmap = nullptr;
    if (epd_gfx_bitmap_load(stream.stream(), &bitmap) != EPD_OK) {
        return false;
    }

    epd_gfx_frame_view_t view = {};
    const epd_err_t ret = epd_gfx_bitmap_get_frame_view(bitmap, &view);
    const bool ok = (ret == EPD_OK) && frameViewToImage(&view, outImage);
    epd_gfx_bitmap_destroy(bitmap);
    return ok;
}

bool generateFrame(const QString& sourcePath, const ProjectBitmapInfo& info,
    epd_gfx_frame_view_t* outView, QString* error)
{
    if (!outView || sourcePath.isEmpty() || info.width == 0U || info.height == 0U) {
        if (error) {
            *error = QStringLiteral("Invalid bitmap generation input.");
        }
        return false;
    }

    QImage image(sourcePath);
    if (image.isNull()) {
        if (error) {
            *error = QStringLiteral("Failed to read source image.");
        }
        return false;
    }

    image = image.scaled(info.width, info.height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    QByteArray sourceData;
    epd_asset_bitmap_source_t source = {};
    if (!FillSourceFromImage(image, &sourceData, &source)) {
        if (error) {
            *error = QStringLiteral("Failed to prepare source image data.");
        }
        return false;
    }

    epd_err_t ret = EPD_OK;
    switch (info.algorithm) {
    case ProjectBitmapAlgorithm::Ordered: {
        epd_asset_bitmap_ordered_config_t config;
        config.format      = info.format;
        config.matrix_size = info.orderedMatrix;
        ret = epd_asset_bitmap_generate_ordered(&source, &config, outView);
        break;
    }

    case ProjectBitmapAlgorithm::BlueNoise: {
        epd_asset_bitmap_blue_noise_config_t config;
        config.format      = info.format;
        config.matrix_size = info.blueNoiseMatrix;
        ret = epd_asset_bitmap_generate_blue_noise(&source, &config, outView);
        break;
    }

    case ProjectBitmapAlgorithm::Random:
        ret = epd_asset_bitmap_generate_random(&source, info.format, outView);
        break;

    case ProjectBitmapAlgorithm::FloydSteinberg:
        ret = epd_asset_bitmap_generate_floyd_steinberg(&source, info.format, outView);
        break;

    case ProjectBitmapAlgorithm::Threshold:
    default: {
        epd_asset_bitmap_threshold_config_t config;
        config.format          = info.format;
        config.black_threshold = info.blackThreshold;
        config.red_threshold   = info.redThreshold;
        config.red_saturation  = info.redSaturation;
        ret = epd_asset_bitmap_generate_threshold(&source, &config, outView);
        break;
    }
    }

    if (ret != EPD_OK) {
        if (error) {
            *error = QStringLiteral("Failed to generate bitmap: %1").arg(errorText(ret));
        }
        return false;
    }

    return true;
}

bool saveFrameView(const QString& path, const epd_gfx_frame_view_t* view, QString* error)
{
    if (path.isEmpty() || !view) {
        if (error) {
            *error = QStringLiteral("Invalid bitmap output.");
        }
        return false;
    }

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        if (error) {
            *error = QStringLiteral("Failed to open bitmap file for writing.");
        }
        return false;
    }

    EpdStreamAdapter stream(&file);
    const epd_err_t ret = epd_asset_bitmap_write_ebm(view, stream.stream());
    if (ret != EPD_OK) {
        if (error) {
            *error = QStringLiteral("Failed to write bitmap file: %1").arg(errorText(ret));
        }
        return false;
    }

    if (!file.commit()) {
        if (error) {
            *error = QStringLiteral("Failed to save bitmap file: commit failed");
        }
        return false;
    }

    return true;
}

bool generateAndSave(const QString& sourcePath, const QString& targetPath,
    const ProjectBitmapInfo& info, QString* error)
{
    epd_gfx_frame_view_t view = {};
    if (!generateFrame(sourcePath, info, &view, error)) {
        return false;
    }

    const bool ok = saveFrameView(targetPath, &view, error);
    epd_asset_bitmap_destroy_frame_view(&view);
    return ok;
}

}

LEKCO_END_NAMESPACE
