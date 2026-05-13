/**
 * @file Project.cpp
 * @brief Project model implementation for the tabula desktop client.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2026-05-12
 * @license MIT
 */

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <epd_gfx/font_asset.h>

#include "project/EpdStreamAdapter.hpp"
#include "project/Project.hpp"

LEKCO_BEGIN_NAMESPACE

BEGIN_NAMESPACE()

constexpr int kManifestVersion = 1;

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

    QJsonObject root;
    root.insert(QStringLiteral("version"), kManifestVersion);
    root.insert(QStringLiteral("screen"), screen);

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

bool Project::createFontResource(const QString& fontName, QString* outFileName, QString* error) const
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

    QSaveFile target(targetPath);
    if (!target.open(QIODevice::WriteOnly)) {
        SetError(error, QStringLiteral("Failed to open font file."));
        return false;
    }

    EpdStreamAdapter stream(&target);

    const QByteArray     identity = fontName.toUtf8();
    epd_gfx_font_asset_t asset    = nullptr;
    epd_err_t            ret      = epd_gfx_font_asset_create(identity.constData(), &asset);
    if (ret == EPD_OK) {
        ret = epd_gfx_font_asset_write_egf(asset, stream.stream());
    }
    epd_gfx_font_asset_destroy(asset);
    if (ret != EPD_OK) {
        SetError(error, QStringLiteral("Failed to write font file."));
        return false;
    }

    if (!target.commit()) {
        SetError(error, QStringLiteral("Failed to save font file."));
        return false;
    }

    if (outFileName) {
        *outFileName = fileName;
    }
    return true;
}

bool Project::removeResource(ProjectResourceType type, const QString& fileName, QString* error) const
{
    if (!validateResourceFileName(type, fileName, error)) {
        return false;
    }

    const QString path = resourcePath(type, fileName);
    if (!QFileInfo(path).isFile()) {
        SetError(error, QStringLiteral("Resource file does not exist."));
        return false;
    }

    if (!QFile::remove(path)) {
        SetError(error, QStringLiteral("Failed to delete resource file."));
        return false;
    }

    return true;
}

QVector<ProjectResource> Project::resources(ProjectResourceType type) const
{
    QVector<ProjectResource> result;
    const QDir dir(resourceDir(type));
    const QFileInfoList files = dir.entryInfoList(QDir::Files | QDir::Readable, QDir::Name);
    for (const QFileInfo& file : files) {
        if (!validateResourceFileName(type, file.fileName())) {
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

QString Project::resourceDir(ProjectResourceType type) const
{
    return QDir(assetsDir()).filePath(directoryName(type));
}

ProjectScreen Project::screen() const
{
    return m_screen;
}

QString Project::displayName(ProjectResourceType type)
{
    switch (type)
    {
    case ProjectResourceType::Fonts:
        return QStringLiteral("Fonts");

    case ProjectResourceType::Images:
        return QStringLiteral("Images");

    case ProjectResourceType::Icons:
        return QStringLiteral("Icons");

    default:
        return QStringLiteral("Unknown");
    }
}

QString Project::directoryName(ProjectResourceType type)
{
    switch (type)
    {
    case ProjectResourceType::Fonts:
        return QStringLiteral("fonts");

    case ProjectResourceType::Images:
        return QStringLiteral("images");

    case ProjectResourceType::Icons:
        return QStringLiteral("icons");

    default:
        return QString();
    }
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
    if (type == ProjectResourceType::Unknown || fileName.trimmed().isEmpty()
        || fileName.contains(QLatin1Char('/')) || fileName.contains(QLatin1Char('\\'))
        || fileName != info.fileName()) {
        SetError(error, QStringLiteral("Invalid resource file name."));
        return false;
    }

    if (type == ProjectResourceType::Fonts
        && info.suffix().compare(QStringLiteral("egf"), Qt::CaseInsensitive) != 0) {
        SetError(error, QStringLiteral("Font resources must use the .egf extension."));
        return false;
    }

    return true;
}

bool Project::ensureDirectories(QString* error) const
{
    QDir root(m_rootDir);
    if (!root.mkpath(QStringLiteral("assets/fonts"))
        || !root.mkpath(QStringLiteral("assets/images"))
        || !root.mkpath(QStringLiteral("assets/icons"))) {
        SetError(error, QStringLiteral("Failed to create project asset directories."));
        return false;
    }
    return true;
}

QString Project::resourcePath(ProjectResourceType type, const QString& fileName) const
{
    return QDir(resourceDir(type)).filePath(fileName);
}

LEKCO_END_NAMESPACE
