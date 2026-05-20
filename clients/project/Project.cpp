/**
 * @file Project.cpp
 * @brief Project model implementation for the tabula desktop client.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2026-05-12
 * @license MIT
 */

#include <stdint.h>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <epd_asset/font_asset.h>

#include "project/BitmapAssetIO.hpp"
#include "project/EpdStreamAdapter.hpp"
#include "project/FontAssetIO.hpp"
#include "project/Project.hpp"

LEKCO_BEGIN_NAMESPACE

BEGIN_NAMESPACE()

constexpr int kManifestVersion = 1;
constexpr auto kSourcesBitmapsDir = "sources/bitmaps";
constexpr auto kSourcesFontsDir = "sources/fonts";

bool HasSameFileContent(const QString& leftPath, const QString& rightPath)
{
    QFile left(leftPath);
    QFile right(rightPath);
    if (!left.open(QIODevice::ReadOnly) || !right.open(QIODevice::ReadOnly) || left.size() != right.size()) {
        return false;
    }

    constexpr qint64 kChunkSize = 64 * 1024;
    while (!left.atEnd()) {
        if (left.read(kChunkSize) != right.read(kChunkSize)) {
            return false;
        }
    }

    return true;
}

QString SafeSourceFileName(const QFileInfo& sourceInfo)
{
    QString baseName = sourceInfo.completeBaseName();
    for (int i = 0; i < baseName.size(); ++i) {
        const ushort value = baseName.at(i).unicode();
        const bool valid = (value >= 'A' && value <= 'Z')
            || (value >= 'a' && value <= 'z')
            || (value >= '0' && value <= '9')
            || value == '_'
            || value == '-';
        if (!valid) {
            baseName[i] = QLatin1Char('_');
        }
    }

    if (baseName.isEmpty()) {
        baseName = QStringLiteral("source");
    }

    return QStringLiteral("%1.%2").arg(baseName, sourceInfo.suffix().toLower());
}

void SetError(QString* error, const QString& message)
{
    if (error) {
        *error = message;
    }
}

bool ValidateSafeName(const QString& name, const QString& typeName, QString* error)
{
    if (name.length() < 1 || name.length() > 64) {
        SetError(error, QStringLiteral("%1 name must contain 1 to 64 characters.").arg(typeName));
        return false;
    }

    for (const QChar ch : name) {
        const ushort value = ch.unicode();
        const bool valid = (value >= 'A' && value <= 'Z')
            || (value >= 'a' && value <= 'z')
            || (value >= '0' && value <= '9')
            || value == '_'
            || value == '-';
        if (!valid) {
            SetError(error, QStringLiteral("%1 name may only contain letters, digits, '_' and '-'.").arg(typeName));
            return false;
        }
    }

    return true;
}

END_NAMESPACE

bool Project::create(const QString& rootDir, const ProjectScreen& screen, QString* error)
{
    if (rootDir.trimmed().isEmpty() || screen.width <= 0 || screen.height <= 0) {
        SetError(error, QStringLiteral("Invalid project configuration."));
        return false;
    }

    QDir root(rootDir);
    if (!root.exists() && !root.mkpath(QStringLiteral("."))) {
        SetError(error, QStringLiteral("Failed to create project directory."));
        return false;
    }

    m_rootDir = QFileInfo(root.absolutePath()).absoluteFilePath();
    m_screen  = screen;
    m_bitmapInfos.clear();
    m_fontSources.clear();

    if (QFileInfo::exists(manifestPath())) {
        SetError(error, QStringLiteral("A manifest already exists in this directory."));
        return false;
    }

    if (!ensureDirectories(error)) {
        return false;
    }

    return save(error);
}

bool Project::open(const QString& path, QString* error)
{
    if (path.trimmed().isEmpty()) {
        SetError(error, QStringLiteral("Project path is empty."));
        return false;
    }

    QFileInfo info(path);
    const QString pathToManifest = info.isDir()
        ? QDir(info.absoluteFilePath()).filePath(QStringLiteral("manifest.json"))
        : info.absoluteFilePath();

    QFile file(pathToManifest);
    if (!file.open(QIODevice::ReadOnly)) {
        SetError(error, QStringLiteral("Failed to open project manifest."));
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        SetError(error, QStringLiteral("Invalid project manifest."));
        return false;
    }

    const QJsonObject root = doc.object();
    if (root.value(QStringLiteral("version")).toInt() != kManifestVersion) {
        SetError(error, QStringLiteral("Unsupported project manifest version."));
        return false;
    }

    const QJsonObject screen = root.value(QStringLiteral("screen")).toObject();
    const int width          = screen.value(QStringLiteral("width")).toInt();
    const int height         = screen.value(QStringLiteral("height")).toInt();
    if (width <= 0 || height <= 0) {
        SetError(error, QStringLiteral("Invalid screen size in manifest."));
        return false;
    }

    m_rootDir       = QFileInfo(pathToManifest).absolutePath();
    m_screen.width  = width;
    m_screen.height = height;
    m_bitmapInfos.clear();
    m_fontSources.clear();

    const QJsonObject bitmaps = root.value(QStringLiteral("bitmaps")).toObject();
    for (auto it = bitmaps.constBegin(); it != bitmaps.constEnd(); ++it) {
        if (!validateResourceFileName(ProjectResourceType::Bitmaps, it.key())) {
            continue;
        }

        ProjectBitmapInfo bitmap = bitmapInfoFromJson(it.value().toObject());
        if (!bitmap.source.isEmpty()
            && !QFileInfo(bitmap.source).isAbsolute()
            && pathIsInProject(absoluteProjectPath(bitmap.source))) {
            m_bitmapInfos.insert(it.key(), bitmap);
        }
    }

    const QJsonObject fonts = root.value(QStringLiteral("fonts")).toObject();
    for (auto it = fonts.constBegin(); it != fonts.constEnd(); ++it) {
        const QJsonObject font = it.value().toObject();
        const QString source   = font.value(QStringLiteral("source")).toString();
        if (validateResourceFileName(ProjectResourceType::Fonts, it.key()) && !source.isEmpty()
            && !QFileInfo(source).isAbsolute() && pathIsInProject(absoluteProjectPath(source))) {
            m_fontSources.insert(it.key(), source);
        }
    }

    return ensureDirectories(error);
}

bool Project::save(QString* error) const
{
    if (!isOpen()) {
        SetError(error, QStringLiteral("Project is not open."));
        return false;
    }

    QJsonObject screen;
    screen.insert(QStringLiteral("width"), m_screen.width);
    screen.insert(QStringLiteral("height"), m_screen.height);

    QJsonObject fonts;
    for (auto it = m_fontSources.constBegin(); it != m_fontSources.constEnd(); ++it) {
        QJsonObject font;
        font.insert(QStringLiteral("source"), it.value());
        fonts.insert(it.key(), font);
    }

    QJsonObject bitmaps;
    for (auto it = m_bitmapInfos.constBegin(); it != m_bitmapInfos.constEnd(); ++it) {
        bitmaps.insert(it.key(), bitmapInfoToJson(it.value()));
    }

    QJsonObject root;
    root.insert(QStringLiteral("version"), kManifestVersion);
    root.insert(QStringLiteral("screen"), screen);
    root.insert(QStringLiteral("bitmaps"), bitmaps);
    root.insert(QStringLiteral("fonts"), fonts);

    QSaveFile file(manifestPath());
    if (!file.open(QIODevice::WriteOnly)) {
        SetError(error, QStringLiteral("Failed to write project manifest."));
        return false;
    }

    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    if (!file.commit()) {
        SetError(error, QStringLiteral("Failed to save project manifest."));
        return false;
    }

    return true;
}

bool Project::createBitmapResource(const QString& bitmapName, const QString& sourceImagePath,
    uint16_t width, uint16_t height, epd_gfx_format_t format, QString* outFileName, QString* error)
{
    if (!validateBitmapName(bitmapName, error)) {
        return false;
    }
    if (width == 0U || height == 0U) {
        SetError(error, QStringLiteral("Bitmap size is invalid."));
        return false;
    }
    if (format != EPD_GFX_FORMAT_NATIVE && format != EPD_GFX_FORMAT_PLANES) {
        SetError(error, QStringLiteral("Bitmap output format is invalid."));
        return false;
    }

    const QString fileName = bitmapName + QStringLiteral(".ebm");
    if (!validateResourceFileName(ProjectResourceType::Bitmaps, fileName, error)) {
        return false;
    }

    const QString targetPath = resourcePath(ProjectResourceType::Bitmaps, fileName);
    if (QFileInfo::exists(targetPath)) {
        SetError(error, QStringLiteral("A bitmap with the same name already exists."));
        return false;
    }

    const QString sourceRelativePath = copyBitmapSource(sourceImagePath, error);
    if (sourceRelativePath.isEmpty()) {
        return false;
    }

    ProjectBitmapInfo info;
    info.source = sourceRelativePath;
    info.width  = width;
    info.height = height;
    info.format = format;

    if (!BitmapAssetIO::generateAndSave(absoluteProjectPath(sourceRelativePath), targetPath, info, error)) {
        QFile::remove(targetPath);
        removeUnusedBitmapSource(sourceRelativePath, fileName);
        return false;
    }

    m_bitmapInfos.insert(fileName, info);
    if (!save(error)) {
        m_bitmapInfos.remove(fileName);
        QFile::remove(targetPath);
        removeUnusedBitmapSource(sourceRelativePath, fileName);
        return false;
    }

    if (outFileName) {
        *outFileName = fileName;
    }
    return true;
}

bool Project::createFontResource(const QString& fontName, const QString& sourceFontPath,
    QString* outFileName, QString* error)
{
    if (!validateFontName(fontName, error)) {
        return false;
    }

    const QString fileName = fontName + QStringLiteral(".egf");
    if (!validateResourceFileName(ProjectResourceType::Fonts, fileName, error)) {
        return false;
    }

    const QString targetPath = resourcePath(ProjectResourceType::Fonts, fileName);
    if (QFileInfo::exists(targetPath)) {
        SetError(error, QStringLiteral("A font with the same name already exists."));
        return false;
    }

    const QString sourceRelativePath = copyFontSource(sourceFontPath, error);
    if (sourceRelativePath.isEmpty()) {
        return false;
    }

    QSaveFile target(targetPath);
    if (!target.open(QIODevice::WriteOnly)) {
        SetError(error, QStringLiteral("Failed to open font file."));
        return false;
    }

    EpdStreamAdapter stream(&target);

    epd_asset_font_asset_t asset = nullptr;
    epd_err_t              ret   = epd_asset_font_asset_create(&asset);
    if (ret == EPD_OK) {
        ret = epd_asset_font_asset_write_egf(asset, stream.stream());
    }
    epd_asset_font_asset_destroy(asset);
    if (ret != EPD_OK) {
        removeUnusedFontSource(sourceRelativePath, fileName);
        SetError(error, QStringLiteral("Failed to write font file."));
        return false;
    }

    if (!target.commit()) {
        removeUnusedFontSource(sourceRelativePath, fileName);
        SetError(error, QStringLiteral("Failed to save font file."));
        return false;
    }

    m_fontSources.insert(fileName, sourceRelativePath);
    if (!save(error)) {
        m_fontSources.remove(fileName);
        QFile::remove(targetPath);
        removeUnusedFontSource(sourceRelativePath, fileName);
        return false;
    }

    if (outFileName) {
        *outFileName = fileName;
    }
    return true;
}

bool Project::removeResource(ProjectResourceType type, const QString& fileName, QString* error)
{
    if (type != ProjectResourceType::Bitmaps && type != ProjectResourceType::Fonts) {
        SetError(error, QStringLiteral("Invalid resource type."));
        return false;
    }

    if (!validateResourceFileName(type, fileName, error)) {
        return false;
    }

    const QString path = resourcePath(type, fileName);
    if (!QFileInfo(path).isFile()) {
        SetError(error, QStringLiteral("Resource file does not exist."));
        return false;
    }

    if (type == ProjectResourceType::Bitmaps) {
        const bool              hasInfo = m_bitmapInfos.contains(fileName);
        const ProjectBitmapInfo info    = m_bitmapInfos.value(fileName);
        if (hasInfo) {
            m_bitmapInfos.remove(fileName);
            if (!save(error)) {
                m_bitmapInfos.insert(fileName, info);
                return false;
            }
        }

        if (!QFile::remove(path)) {
            if (hasInfo) {
                m_bitmapInfos.insert(fileName, info);
                (void)save();
            }
            SetError(error, QStringLiteral("Failed to delete resource file."));
            return false;
        }

        if (hasInfo && !removeUnusedBitmapSource(info.source, fileName)) {
            SetError(error, QStringLiteral("Failed to delete unused source image."));
            return false;
        }

        return true;
    }

    const bool    hasSourceEntry     = m_fontSources.contains(fileName);
    const QString sourceRelativePath = m_fontSources.value(fileName);
    if (hasSourceEntry) {
        m_fontSources.remove(fileName);
        if (!save(error)) {
            m_fontSources.insert(fileName, sourceRelativePath);
            return false;
        }
    }

    if (!QFile::remove(path)) {
        if (hasSourceEntry) {
            m_fontSources.insert(fileName, sourceRelativePath);
            (void)save();
        }
        SetError(error, QStringLiteral("Failed to delete resource file."));
        return false;
    }

    if (!removeUnusedFontSource(sourceRelativePath, fileName)) {
        SetError(error, QStringLiteral("Failed to delete unused source font."));
        return false;
    }

    return true;
}

bool Project::exportAssets(const QString& targetDir, qint64* outBytes, QString* error) const
{
    if (targetDir.trimmed().isEmpty()) {
        SetError(error, QStringLiteral("Export directory is empty."));
        return false;
    }

    const QString sourcePath = assetsDir();
    if (!QFileInfo(sourcePath).isDir()) {
        SetError(error, QStringLiteral("Project assets directory does not exist."));
        return false;
    }

    const QFileInfo targetInfo(targetDir);
    if (QFileInfo(sourcePath).absoluteFilePath() == targetInfo.absoluteFilePath()) {
        SetError(error, QStringLiteral("Export target cannot be the project assets directory."));
        return false;
    }

    QDir target(targetDir);
    if (target.exists() && !target.removeRecursively()) {
        SetError(error, QStringLiteral("Failed to remove old export directory."));
        return false;
    }

    QDir targetParent(targetInfo.absolutePath());
    if (!targetParent.mkpath(targetInfo.fileName())) {
        SetError(error, QStringLiteral("Failed to create export directory."));
        return false;
    }

    qint64           totalBytes = 0;
    QDirIterator     it(sourcePath, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    const QDir       sourceDir(sourcePath);
    const QDir       targetDirHandle(targetDir);
    while (it.hasNext()) {
        const QString inputPath    = it.next();
        const QString relativePath = sourceDir.relativeFilePath(inputPath);
        const QString outputPath   = targetDirHandle.filePath(relativePath);
        if (!QDir().mkpath(QFileInfo(outputPath).absolutePath()) || !QFile::copy(inputPath, outputPath)) {
            SetError(error, QStringLiteral("Failed to copy asset file."));
            return false;
        }
        totalBytes += QFileInfo(outputPath).size();
    }

    if (outBytes) {
        *outBytes = totalBytes;
    }
    return true;
}

QVector<ProjectResource> Project::resources(ProjectResourceType type) const
{
    QVector<ProjectResource> result;
    if (type != ProjectResourceType::Bitmaps && type != ProjectResourceType::Fonts) {
        return result;
    }

    const QDir dir(resourceDir(type));
    const QFileInfoList files = dir.entryInfoList(QDir::Files | QDir::Readable, QDir::Name);
    for (const QFileInfo& file : files) {
        if (!validateResourceFileName(type, file.fileName())) {
            continue;
        }
        if (type == ProjectResourceType::Bitmaps && !isValidBitmapResource(file.absoluteFilePath())) {
            continue;
        }
        if (type == ProjectResourceType::Fonts && !isValidFontResource(file.absoluteFilePath())) {
            continue;
        }

        ProjectResource resource;
        resource.type         = type;
        resource.fileName     = file.fileName();
        resource.absolutePath = file.absoluteFilePath();
        result.append(resource);
    }
    return result;
}

bool Project::isOpen() const
{
    return !m_rootDir.isEmpty();
}

QString Project::rootDir() const
{
    return m_rootDir;
}

QString Project::manifestPath() const
{
    return QDir(m_rootDir).filePath(QStringLiteral("manifest.json"));
}

QString Project::assetsDir() const
{
    return QDir(m_rootDir).filePath(QStringLiteral("assets"));
}

QString Project::sourcesDir() const
{
    return QDir(m_rootDir).filePath(QStringLiteral("sources"));
}

QString Project::resourceDir(ProjectResourceType type) const
{
    switch (type) {
    case ProjectResourceType::Bitmaps:
        return QDir(assetsDir()).filePath(QStringLiteral("bitmaps"));

    case ProjectResourceType::Fonts:
        return QDir(assetsDir()).filePath(QStringLiteral("fonts"));

    default:
        return QString();
    }
}

ProjectScreen Project::screen() const
{
    return m_screen;
}

bool Project::bitmapInfo(const QString& bitmapFileName, ProjectBitmapInfo* outInfo) const
{
    if (!outInfo || !m_bitmapInfos.contains(bitmapFileName)) {
        return false;
    }

    *outInfo = m_bitmapInfos.value(bitmapFileName);
    return true;
}

bool Project::setBitmapInfo(const QString& bitmapFileName, const ProjectBitmapInfo& info,
    QString* error)
{
    if (!validateResourceFileName(ProjectResourceType::Bitmaps, bitmapFileName, error)) {
        return false;
    }
    if (info.source.isEmpty() || QFileInfo(info.source).isAbsolute()
        || !pathIsInProject(absoluteProjectPath(info.source))) {
        SetError(error, QStringLiteral("Invalid bitmap source path."));
        return false;
    }
    if (info.width == 0U || info.height == 0U) {
        SetError(error, QStringLiteral("Invalid bitmap size."));
        return false;
    }
    if (info.format != EPD_GFX_FORMAT_NATIVE && info.format != EPD_GFX_FORMAT_PLANES) {
        SetError(error, QStringLiteral("Invalid bitmap format."));
        return false;
    }

    const bool              hadInfo = m_bitmapInfos.contains(bitmapFileName);
    const ProjectBitmapInfo oldInfo = m_bitmapInfos.value(bitmapFileName);
    m_bitmapInfos.insert(bitmapFileName, info);
    if (!save(error)) {
        if (hadInfo) {
            m_bitmapInfos.insert(bitmapFileName, oldInfo);
        } else {
            m_bitmapInfos.remove(bitmapFileName);
        }
        return false;
    }

    return true;
}

QString Project::bitmapSourcePath(const QString& bitmapFileName) const
{
    ProjectBitmapInfo info;
    if (!bitmapInfo(bitmapFileName, &info) || info.source.isEmpty()
        || QFileInfo(info.source).isAbsolute()) {
        return QString();
    }

    const QString sourcePath = absoluteProjectPath(info.source);
    return pathIsInProject(sourcePath) ? sourcePath : QString();
}

bool Project::bitmapSourceExists(const QString& bitmapFileName) const
{
    const QString sourcePath = bitmapSourcePath(bitmapFileName);
    return !sourcePath.isEmpty() && QFileInfo(sourcePath).isFile();
}

QString Project::fontSourcePath(const QString& fontFileName) const
{
    const QString sourceRelativePath = m_fontSources.value(fontFileName);
    if (sourceRelativePath.isEmpty() || QFileInfo(sourceRelativePath).isAbsolute()) {
        return QString();
    }

    const QString sourcePath = absoluteProjectPath(sourceRelativePath);
    return pathIsInProject(sourcePath) ? sourcePath : QString();
}

bool Project::fontSourceExists(const QString& fontFileName) const
{
    const QString sourcePath = fontSourcePath(fontFileName);
    return !sourcePath.isEmpty() && QFileInfo(sourcePath).isFile();
}

bool Project::validateBitmapName(const QString& bitmapName, QString* error)
{
    return ValidateSafeName(bitmapName, QStringLiteral("Bitmap"), error);
}

bool Project::validateFontName(const QString& fontName, QString* error)
{
    return ValidateSafeName(fontName, QStringLiteral("Font"), error);
}

bool Project::validateResourceFileName(ProjectResourceType type, const QString& fileName, QString* error)
{
    const QFileInfo info(fileName);
    if ((type != ProjectResourceType::Bitmaps && type != ProjectResourceType::Fonts)
        || fileName.trimmed().isEmpty()
        || fileName.contains(QLatin1Char('/')) || fileName.contains(QLatin1Char('\\'))
        || fileName != info.fileName()) {
        SetError(error, QStringLiteral("Invalid resource file name."));
        return false;
    }

    if (type == ProjectResourceType::Bitmaps
        && info.suffix().compare(QStringLiteral("ebm"), Qt::CaseInsensitive) != 0) {
        SetError(error, QStringLiteral("Bitmap resources must use the .ebm extension."));
        return false;
    }
    if (type == ProjectResourceType::Fonts
        && info.suffix().compare(QStringLiteral("egf"), Qt::CaseInsensitive) != 0) {
        SetError(error, QStringLiteral("Font resources must use the .egf extension."));
        return false;
    }

    return true;
}

QString Project::bitmapAlgorithmToString(ProjectBitmapAlgorithm algorithm)
{
    switch (algorithm) {
    case ProjectBitmapAlgorithm::Ordered:
        return QStringLiteral("ordered");

    case ProjectBitmapAlgorithm::BlueNoise:
        return QStringLiteral("blue_noise");

    case ProjectBitmapAlgorithm::Random:
        return QStringLiteral("random");

    case ProjectBitmapAlgorithm::FloydSteinberg:
        return QStringLiteral("floyd_steinberg");

    case ProjectBitmapAlgorithm::Threshold:
    default:
        return QStringLiteral("threshold");
    }
}

ProjectBitmapAlgorithm Project::bitmapAlgorithmFromString(const QString& value)
{
    if (value == QStringLiteral("ordered")) {
        return ProjectBitmapAlgorithm::Ordered;
    }
    if (value == QStringLiteral("blue_noise")) {
        return ProjectBitmapAlgorithm::BlueNoise;
    }
    if (value == QStringLiteral("random")) {
        return ProjectBitmapAlgorithm::Random;
    }
    if (value == QStringLiteral("floyd_steinberg")) {
        return ProjectBitmapAlgorithm::FloydSteinberg;
    }
    return ProjectBitmapAlgorithm::Threshold;
}

QString Project::bitmapFormatToString(epd_gfx_format_t format)
{
    return format == EPD_GFX_FORMAT_PLANES
        ? QStringLiteral("planes")
        : QStringLiteral("native");
}

epd_gfx_format_t Project::bitmapFormatFromString(const QString& value)
{
    return value == QStringLiteral("planes")
        ? EPD_GFX_FORMAT_PLANES
        : EPD_GFX_FORMAT_NATIVE;
}

bool Project::ensureDirectories(QString* error) const
{
    QDir root(m_rootDir);
    if (!root.mkpath(QStringLiteral("assets/bitmaps"))
        || !root.mkpath(QStringLiteral("assets/fonts"))
        || !root.mkpath(QString::fromLatin1(kSourcesBitmapsDir))
        || !root.mkpath(QString::fromLatin1(kSourcesFontsDir))) {
        SetError(error, QStringLiteral("Failed to create project asset directories."));
        return false;
    }
    return true;
}

bool Project::isValidBitmapResource(const QString& path) const
{
    return BitmapAssetIO::isValidEbmFile(path);
}

bool Project::isValidFontResource(const QString& path) const
{
    return FontAssetIO::isValidEgfFile(path);
}

QString Project::resourcePath(ProjectResourceType type, const QString& fileName) const
{
    return QDir(resourceDir(type)).filePath(fileName);
}

QString Project::copyBitmapSource(const QString& sourceImagePath, QString* error) const
{
    const QFileInfo sourceInfo(sourceImagePath);
    if (!sourceInfo.isFile()) {
        SetError(error, QStringLiteral("Source image file does not exist."));
        return QString();
    }

    QImage image(sourceInfo.absoluteFilePath());
    if (image.isNull()) {
        SetError(error, QStringLiteral("Source image file is not supported."));
        return QString();
    }

    QDir root(m_rootDir);
    if (!root.mkpath(QString::fromLatin1(kSourcesBitmapsDir))) {
        SetError(error, QStringLiteral("Failed to create source bitmap directory."));
        return QString();
    }

    const QString fileName   = SafeSourceFileName(sourceInfo);
    const QString targetPath = QDir(sourcesDir()).filePath(QStringLiteral("bitmaps/%1").arg(fileName));
    if (QFileInfo::exists(targetPath)) {
        if (HasSameFileContent(sourceInfo.absoluteFilePath(), targetPath)) {
            return QStringLiteral("%1/%2").arg(QString::fromLatin1(kSourcesBitmapsDir), fileName);
        }

        SetError(error, QStringLiteral("A different source image with the same name already exists."));
        return QString();
    }

    if (!QFile::copy(sourceInfo.absoluteFilePath(), targetPath)) {
        SetError(error, QStringLiteral("Failed to copy source image file."));
        return QString();
    }

    return QStringLiteral("%1/%2").arg(QString::fromLatin1(kSourcesBitmapsDir), fileName);
}

QString Project::copyFontSource(const QString& sourceFontPath, QString* error) const
{
    const QFileInfo sourceInfo(sourceFontPath);
    if (!sourceInfo.isFile()) {
        SetError(error, QStringLiteral("Source font file does not exist."));
        return QString();
    }

    const QString suffix = sourceInfo.suffix();
    if (suffix.compare(QStringLiteral("ttf"), Qt::CaseInsensitive) != 0
        && suffix.compare(QStringLiteral("otf"), Qt::CaseInsensitive) != 0) {
        SetError(error, QStringLiteral("Source font file must use the .ttf or .otf extension."));
        return QString();
    }

    QDir root(m_rootDir);
    if (!root.mkpath(QString::fromLatin1(kSourcesFontsDir))) {
        SetError(error, QStringLiteral("Failed to create source font directory."));
        return QString();
    }

    const QString fileName   = SafeSourceFileName(sourceInfo);
    const QString targetPath = QDir(sourcesDir()).filePath(QStringLiteral("fonts/%1").arg(fileName));
    if (QFileInfo::exists(targetPath)) {
        if (HasSameFileContent(sourceInfo.absoluteFilePath(), targetPath)) {
            return QStringLiteral("%1/%2").arg(QString::fromLatin1(kSourcesFontsDir), fileName);
        }

        SetError(error, QStringLiteral("A different source font with the same name already exists."));
        return QString();
    }

    if (!QFile::copy(sourceInfo.absoluteFilePath(), targetPath)) {
        SetError(error, QStringLiteral("Failed to copy source font file."));
        return QString();
    }

    return QStringLiteral("%1/%2").arg(QString::fromLatin1(kSourcesFontsDir), fileName);
}

QString Project::absoluteProjectPath(const QString& relativePath) const
{
    return QFileInfo(QDir(m_rootDir).filePath(relativePath)).absoluteFilePath();
}

bool Project::pathIsInProject(const QString& path) const
{
    const QDir    root(QFileInfo(m_rootDir).absoluteFilePath());
    const QString relativePath = root.relativeFilePath(QFileInfo(path).absoluteFilePath());
    return relativePath == QStringLiteral(".")
        || (!relativePath.startsWith(QStringLiteral("../")) && !relativePath.startsWith(QStringLiteral("..\\"))
            && relativePath != QStringLiteral("..") && !QFileInfo(relativePath).isAbsolute());
}

bool Project::sourceUsedByOtherBitmap(const QString& sourceRelativePath, const QString& ignoredBitmapFileName) const
{
    for (auto it = m_bitmapInfos.constBegin(); it != m_bitmapInfos.constEnd(); ++it) {
        if (it.key() != ignoredBitmapFileName && it.value().source == sourceRelativePath) {
            return true;
        }
    }
    return false;
}

bool Project::removeUnusedBitmapSource(const QString& sourceRelativePath, const QString& ignoredBitmapFileName) const
{
    if (sourceRelativePath.isEmpty() || sourceUsedByOtherBitmap(sourceRelativePath, ignoredBitmapFileName)) {
        return true;
    }

    const QString sourcePath = absoluteProjectPath(sourceRelativePath);
    return !pathIsInProject(sourcePath) || !QFileInfo(sourcePath).isFile() || QFile::remove(sourcePath);
}

bool Project::sourceUsedByOtherFont(const QString& sourceRelativePath, const QString& ignoredFontFileName) const
{
    for (auto it = m_fontSources.constBegin(); it != m_fontSources.constEnd(); ++it) {
        if (it.key() != ignoredFontFileName && it.value() == sourceRelativePath) {
            return true;
        }
    }
    return false;
}

bool Project::removeUnusedFontSource(const QString& sourceRelativePath, const QString& ignoredFontFileName) const
{
    if (sourceRelativePath.isEmpty() || sourceUsedByOtherFont(sourceRelativePath, ignoredFontFileName)) {
        return true;
    }

    const QString sourcePath = absoluteProjectPath(sourceRelativePath);
    return !pathIsInProject(sourcePath) || !QFileInfo(sourcePath).isFile() || QFile::remove(sourcePath);
}

QJsonObject Project::bitmapParamsToJson(const ProjectBitmapInfo& info)
{
    QJsonObject params;
    switch (info.algorithm) {
    case ProjectBitmapAlgorithm::Ordered:
        params.insert(QStringLiteral("orderedMatrix"), static_cast<int>(info.orderedMatrix));
        break;

    case ProjectBitmapAlgorithm::BlueNoise:
        params.insert(QStringLiteral("blueNoiseMatrix"), static_cast<int>(info.blueNoiseMatrix));
        break;

    case ProjectBitmapAlgorithm::Threshold:
        params.insert(QStringLiteral("blackThreshold"), info.blackThreshold);
        params.insert(QStringLiteral("redThreshold"), info.redThreshold);
        params.insert(QStringLiteral("redSaturation"), info.redSaturation);
        break;

    case ProjectBitmapAlgorithm::Random:
    case ProjectBitmapAlgorithm::FloydSteinberg:
    default:
        break;
    }
    return params;
}

static uint8_t BitmapParamUInt8(const QJsonObject& params, const QString& key, uint8_t fallback)
{
    const int value = params.value(key).toInt(fallback);
    return value >= 0 && value <= UINT8_MAX ? static_cast<uint8_t>(value) : fallback;
}

ProjectBitmapInfo Project::bitmapInfoFromJson(const QJsonObject& object)
{
    ProjectBitmapInfo info;
    const int width = object.value(QStringLiteral("width")).toInt();
    const int height = object.value(QStringLiteral("height")).toInt();
    info.source = object.value(QStringLiteral("source")).toString();
    info.width  = width > 0 && width <= UINT16_MAX ? static_cast<uint16_t>(width) : 0U;
    info.height = height > 0 && height <= UINT16_MAX ? static_cast<uint16_t>(height) : 0U;
    info.format    = bitmapFormatFromString(object.value(QStringLiteral("format")).toString());
    info.algorithm = bitmapAlgorithmFromString(object.value(QStringLiteral("algorithm")).toString());

    const QJsonObject params = object.value(QStringLiteral("params")).toObject();
    info.blackThreshold = BitmapParamUInt8(params, QStringLiteral("blackThreshold"), info.blackThreshold);
    info.redThreshold = BitmapParamUInt8(params, QStringLiteral("redThreshold"), info.redThreshold);
    info.redSaturation = BitmapParamUInt8(params, QStringLiteral("redSaturation"), info.redSaturation);

    const int orderedMatrix = params.value(QStringLiteral("orderedMatrix")).toInt(
        static_cast<int>(info.orderedMatrix));
    if (orderedMatrix >= EPD_ASSET_BITMAP_ORDERED_MATRIX_2X2
        && orderedMatrix <= EPD_ASSET_BITMAP_ORDERED_MATRIX_8X8) {
        info.orderedMatrix = static_cast<epd_asset_bitmap_ordered_matrix_size_t>(orderedMatrix);
    }

    const int blueNoiseMatrix = params.value(QStringLiteral("blueNoiseMatrix")).toInt(
        static_cast<int>(info.blueNoiseMatrix));
    if (blueNoiseMatrix >= EPD_ASSET_BITMAP_BLUE_NOISE_MATRIX_64X64
        && blueNoiseMatrix <= EPD_ASSET_BITMAP_BLUE_NOISE_MATRIX_256X256) {
        info.blueNoiseMatrix = static_cast<epd_asset_bitmap_blue_noise_matrix_size_t>(blueNoiseMatrix);
    }

    return info;
}

QJsonObject Project::bitmapInfoToJson(const ProjectBitmapInfo& info)
{
    QJsonObject object;
    object.insert(QStringLiteral("source"), info.source);
    object.insert(QStringLiteral("width"), info.width);
    object.insert(QStringLiteral("height"), info.height);
    object.insert(QStringLiteral("format"), bitmapFormatToString(info.format));
    object.insert(QStringLiteral("algorithm"), bitmapAlgorithmToString(info.algorithm));
    object.insert(QStringLiteral("params"), bitmapParamsToJson(info));
    return object;
}

LEKCO_END_NAMESPACE
