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

#include <QMap>
#include <QString>
#include <QVector>

#include "common/Common.h"

LEKCO_BEGIN_NAMESPACE

enum class ProjectResourceType {
    Unknown = 0,
    Fonts,
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

class Project
{
public:
    bool create(const QString& rootDir, const ProjectScreen& screen, QString* error = nullptr);
    bool open(const QString& path, QString* error = nullptr);
    bool save(QString* error = nullptr) const;

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
    QString fontSourcePath(const QString& fontFileName) const;
    bool fontSourceExists(const QString& fontFileName) const;

    static bool validateFontName(const QString& fontName, QString* error = nullptr);
    static bool validateResourceFileName(ProjectResourceType type, const QString& fileName,
        QString* error = nullptr);

private:
    bool ensureDirectories(QString* error = nullptr) const;
    bool isValidFontResource(const QString& path) const;
    QString resourcePath(ProjectResourceType type, const QString& fileName) const;
    QString copyFontSource(const QString& sourceFontPath, QString* error = nullptr) const;
    QString absoluteProjectPath(const QString& relativePath) const;
    bool pathIsInProject(const QString& path) const;
    bool sourceUsedByOtherFont(const QString& sourceRelativePath, const QString& ignoredFontFileName) const;
    bool removeUnusedFontSource(const QString& sourceRelativePath, const QString& ignoredFontFileName) const;

    QString       m_rootDir;
    ProjectScreen m_screen;
    QMap<QString, QString> m_fontSources;
};

LEKCO_END_NAMESPACE

#endif // !_PROJECT_HPP_
