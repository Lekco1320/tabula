/**
 * @file Project.cpp
 * @brief Project model implementation for the tabula desktop client.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2026-05-12
 * @license MIT
 */

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>

#include "project/Project.hpp"

LEKCO_BEGIN_NAMESPACE

namespace {

constexpr int kManifestVersion = 1;
constexpr int kCopyBufferSize  = 64 * 1024;

void SetError(QString* error, const QString& message)
{
    if (error) {
        *error = message;
    }
}

bool IsSameFileName(const QString& lhs, const QString& rhs)
{
    return lhs.compare(rhs, Qt::CaseInsensitive) == 0;
}

} // namespace

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

bool Project::addResourceFromFile(ProjectResourceType type, const QString& sourcePath, QString* error) const
{
    const QFileInfo sourceInfo(sourcePath);
    if (!sourceInfo.isFile()) {
        SetError(error, QStringLiteral("Source resource is not a file."));
        return false;
    }

    const QString fileName = sourceInfo.fileName();
    if (!validateResourceFileName(type, fileName, error)) {
        return false;
    }

    const QString targetPath = resourcePath(type, fileName);
    if (QFileInfo::exists(targetPath)) {
        SetError(error, QStringLiteral("A resource with the same file name already exists."));
        return false;
    }

    return writeFileFromFile(sourcePath, targetPath, error);
}

bool Project::updateResource(ProjectResourceType type, const QString& oldFileName,
    const QString& newFileName, const QString& replacementPath, QString* error) const
{
    if (!validateResourceFileName(type, oldFileName, error)
        || !validateResourceFileName(type, newFileName, error)) {
        return false;
    }

    const QString oldPath = resourcePath(type, oldFileName);
    const QString newPath = resourcePath(type, newFileName);
    if (!QFileInfo(oldPath).isFile()) {
        SetError(error, QStringLiteral("Resource file does not exist."));
        return false;
    }

    const bool renameFile  = !IsSameFileName(oldFileName, newFileName);
    const bool replaceFile = !replacementPath.trimmed().isEmpty();
    if (renameFile && QFileInfo::exists(newPath)) {
        SetError(error, QStringLiteral("A resource with the new file name already exists."));
        return false;
    }

    if (replaceFile) {
        if (!QFileInfo(replacementPath).isFile()) {
            SetError(error, QStringLiteral("Replacement file does not exist."));
            return false;
        }

        if (!writeFileFromFile(replacementPath, newPath, error)) {
            return false;
        }

        if (renameFile && !QFile::remove(oldPath)) {
            SetError(error, QStringLiteral("Failed to remove the old resource file."));
            return false;
        }
        return true;
    }

    if (renameFile && !QFile::rename(oldPath, newPath)) {
        SetError(error, QStringLiteral("Failed to rename resource file."));
        return false;
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

QString Project::fileDialogFilter(ProjectResourceType type)
{
    switch (type)
    {
    case ProjectResourceType::Fonts:
        return QStringLiteral("EGF Fonts (*.egf)");

    default:
        return QStringLiteral("All Files (*)");
    }
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

bool Project::writeFileFromFile(const QString& sourcePath, const QString& targetPath, QString* error) const
{
    QFile source(sourcePath);
    if (!source.open(QIODevice::ReadOnly)) {
        SetError(error, QStringLiteral("Failed to open source file."));
        return false;
    }

    QSaveFile target(targetPath);
    if (!target.open(QIODevice::WriteOnly)) {
        SetError(error, QStringLiteral("Failed to open target file."));
        return false;
    }

    while (!source.atEnd()) {
        const QByteArray chunk = source.read(kCopyBufferSize);
        if (chunk.isEmpty() && source.error() != QFile::NoError) {
            SetError(error, QStringLiteral("Failed to read source file."));
            return false;
        }
        if (target.write(chunk) != chunk.size()) {
            SetError(error, QStringLiteral("Failed to write target file."));
            return false;
        }
    }

    if (!target.commit()) {
        SetError(error, QStringLiteral("Failed to save target file."));
        return false;
    }

    return true;
}

QString Project::resourcePath(ProjectResourceType type, const QString& fileName) const
{
    return QDir(resourceDir(type)).filePath(fileName);
}

LEKCO_END_NAMESPACE
