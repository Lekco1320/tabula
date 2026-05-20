/**
 * @file Project.hpp
 * @brief Project model for the tabula desktop client.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2026-05-12
 * @license MIT
 */

#pragma once

#ifndef _PROJECT_HPP_
#define _PROJECT_HPP_

#include <QJsonObject>
#include <QMap>
#include <QString>
#include <QVector>
#include <epd_asset/bitmap_generator.h>
#include <epd_gfx/common.h>

#include "common/Common.h"

LEKCO_BEGIN_NAMESPACE

enum class ProjectResourceType {
    Unknown = 0,
    Bitmaps,
    Fonts,
};

enum class ProjectBitmapAlgorithm {
    Threshold = 0,
    Ordered,
    BlueNoise,
    Random,
    FloydSteinberg,
};

struct ProjectScreen {
    int width  = 640;
    int height = 384;
};

struct ProjectResource {
    ProjectResourceType type = ProjectResourceType::Unknown;
    QString             fileName;
    QString             absolutePath;
};

struct ProjectBitmapInfo {
    QString                                source;
    uint16_t                               width          = 0U;
    uint16_t                               height         = 0U;
    epd_gfx_format_t                       format         = EPD_GFX_FORMAT_NATIVE;
    ProjectBitmapAlgorithm                 algorithm      = ProjectBitmapAlgorithm::Threshold;
    uint8_t                                blackThreshold = 128U;
    uint8_t                                redThreshold   = 32U;
    uint8_t                                redSaturation  = 40U;
    epd_asset_bitmap_ordered_matrix_size_t orderedMatrix  =
        EPD_ASSET_BITMAP_ORDERED_MATRIX_8X8;
    epd_asset_bitmap_blue_noise_matrix_size_t blueNoiseMatrix =
        EPD_ASSET_BITMAP_BLUE_NOISE_MATRIX_128X128;
};

class Project
{
public:
    bool create(const QString& rootDir, const ProjectScreen& screen, QString* error = nullptr);
    bool open(const QString& path, QString* error = nullptr);
    bool save(QString* error = nullptr) const;

    bool createBitmapResource(const QString& bitmapName, const QString& sourceImagePath,
        uint16_t width, uint16_t height, epd_gfx_format_t format,
        QString* outFileName = nullptr, QString* error = nullptr);
    bool createFontResource(const QString& fontName, const QString& sourceFontPath,
        QString* outFileName = nullptr, QString* error = nullptr);
    bool removeResource(ProjectResourceType type, const QString& fileName, QString* error = nullptr);
    bool exportAssets(const QString& targetDir, qint64* outBytes = nullptr, QString* error = nullptr) const;

    QVector<ProjectResource> resources(ProjectResourceType type) const;

    bool isOpen() const;
    QString rootDir() const;
    QString manifestPath() const;
    QString assetsDir() const;
    QString sourcesDir() const;
    QString resourceDir(ProjectResourceType type) const;
    ProjectScreen screen() const;
    bool bitmapInfo(const QString& bitmapFileName, ProjectBitmapInfo* outInfo) const;
    bool setBitmapInfo(const QString& bitmapFileName, const ProjectBitmapInfo& info,
        QString* error = nullptr);
    QString bitmapSourcePath(const QString& bitmapFileName) const;
    bool bitmapSourceExists(const QString& bitmapFileName) const;
    QString fontSourcePath(const QString& fontFileName) const;
    bool fontSourceExists(const QString& fontFileName) const;

    static bool validateBitmapName(const QString& bitmapName, QString* error = nullptr);
    static bool validateFontName(const QString& fontName, QString* error = nullptr);
    static bool validateResourceFileName(ProjectResourceType type, const QString& fileName,
        QString* error = nullptr);
    static QString bitmapAlgorithmToString(ProjectBitmapAlgorithm algorithm);
    static ProjectBitmapAlgorithm bitmapAlgorithmFromString(const QString& value);
    static QString bitmapFormatToString(epd_gfx_format_t format);
    static epd_gfx_format_t bitmapFormatFromString(const QString& value);

private:
    bool ensureDirectories(QString* error = nullptr) const;
    bool isValidBitmapResource(const QString& path) const;
    bool isValidFontResource(const QString& path) const;
    QString resourcePath(ProjectResourceType type, const QString& fileName) const;
    QString copyBitmapSource(const QString& sourceImagePath, QString* error = nullptr) const;
    QString copyFontSource(const QString& sourceFontPath, QString* error = nullptr) const;
    QString absoluteProjectPath(const QString& relativePath) const;
    bool pathIsInProject(const QString& path) const;
    bool sourceUsedByOtherBitmap(const QString& sourceRelativePath, const QString& ignoredBitmapFileName) const;
    bool removeUnusedBitmapSource(const QString& sourceRelativePath, const QString& ignoredBitmapFileName) const;
    bool sourceUsedByOtherFont(const QString& sourceRelativePath, const QString& ignoredFontFileName) const;
    bool removeUnusedFontSource(const QString& sourceRelativePath, const QString& ignoredFontFileName) const;
    static QJsonObject bitmapParamsToJson(const ProjectBitmapInfo& info);
    static ProjectBitmapInfo bitmapInfoFromJson(const QJsonObject& object);
    static QJsonObject bitmapInfoToJson(const ProjectBitmapInfo& info);

    QString       m_rootDir;
    ProjectScreen m_screen;
    QMap<QString, ProjectBitmapInfo> m_bitmapInfos;
    QMap<QString, QString> m_fontSources;
};

LEKCO_END_NAMESPACE

#endif // !_PROJECT_HPP_
