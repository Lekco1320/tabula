/**
 * @file Project.cpp
 * @brief Project model implementation for the tabula desktop client.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2026-05-12
 * @license MIT
 */

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <epd_asset/font_asset.h>

#include "project/EpdStreamAdapter.hpp"
#include "project/FontAssetIO.hpp"
#include "project/Project.hpp"

LEKCO_BEGIN_NAMESPACE

BEGIN_NAMESPACE()

constexpr int kManifestVersion = 1;
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
    m_fontSources.clear();

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

    QJsonObject root;
    root.insert(QStringLiteral("version"), kManifestVersion);
    root.insert(QStringLiteral("screen"), screen);
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
    if (type != ProjectResourceType::Fonts) {
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
    if (type != ProjectResourceType::Fonts) {
        return result;
    }

    const QDir dir(resourceDir(type));
    const QFileInfoList files = dir.entryInfoList(QDir::Files | QDir::Readable, QDir::Name);
    for (const QFileInfo& file : files) {
        if (!validateResourceFileName(type, file.fileName())) {
            continue;
        }
        if (!isValidFontResource(file.absoluteFilePath())) {
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
    return type == ProjectResourceType::Fonts
        ? QDir(assetsDir()).filePath(QStringLiteral("fonts"))
        : QString();
}

ProjectScreen Project::screen() const
{
    return m_screen;
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

bool Project::validateFontName(const QString& fontName, QString* error)
{
    if (fontName.length() < 1 || fontName.length() > 64) {
        SetError(error, QStringLiteral("Font name must contain 1 to 64 characters."));
        return false;
    }

    for (const QChar ch : fontName) {
        const ushort value = ch.unicode();
        const bool valid = (value >= 'A' && value <= 'Z')
            || (value >= 'a' && value <= 'z')
            || (value >= '0' && value <= '9')
            || value == '_'
            || value == '-';
        if (!valid) {
            SetError(error, QStringLiteral("Font name may only contain letters, digits, '_' and '-'."));
            return false;
        }
    }

    return true;
}

bool Project::validateResourceFileName(ProjectResourceType type, const QString& fileName, QString* error)
{
    const QFileInfo info(fileName);
    if (type != ProjectResourceType::Fonts || fileName.trimmed().isEmpty()
        || fileName.contains(QLatin1Char('/')) || fileName.contains(QLatin1Char('\\'))
        || fileName != info.fileName()) {
        SetError(error, QStringLiteral("Invalid resource file name."));
        return false;
    }

    if (info.suffix().compare(QStringLiteral("egf"), Qt::CaseInsensitive) != 0) {
        SetError(error, QStringLiteral("Font resources must use the .egf extension."));
        return false;
    }

    return true;
}

bool Project::ensureDirectories(QString* error) const
{
    QDir root(m_rootDir);
    if (!root.mkpath(QStringLiteral("assets/fonts"))
        || !root.mkpath(QString::fromLatin1(kSourcesFontsDir))) {
        SetError(error, QStringLiteral("Failed to create project asset directories."));
        return false;
    }
    return true;
}

bool Project::isValidFontResource(const QString& path) const
{
    return FontAssetIO::isValidEgfFile(path);
}

QString Project::resourcePath(ProjectResourceType type, const QString& fileName) const
{
    return QDir(resourceDir(type)).filePath(fileName);
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

LEKCO_END_NAMESPACE
