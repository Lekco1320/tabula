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

#include <QString>
#include <QVector>

#include "common/Common.h"

LEKCO_BEGIN_NAMESPACE

enum class ProjectResourceType {
    Unknown = 0,
    Fonts,
    Images,
    Icons,
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

    bool createFontResource(const QString& fontName, QString* outFileName = nullptr,
        QString* error = nullptr) const;
    bool removeResource(ProjectResourceType type, const QString& fileName, QString* error = nullptr) const;

    QVector<ProjectResource> resources(ProjectResourceType type) const;

    bool isOpen() const;
    QString rootDir() const;
    QString manifestPath() const;
    QString assetsDir() const;
    QString resourceDir(ProjectResourceType type) const;
    ProjectScreen screen() const;

    static QString displayName(ProjectResourceType type);
    static QString directoryName(ProjectResourceType type);
    static bool validateFontName(const QString& fontName, QString* error = nullptr);
    static bool validateResourceFileName(ProjectResourceType type, const QString& fileName,
        QString* error = nullptr);

private:
    bool ensureDirectories(QString* error = nullptr) const;
    QString resourcePath(ProjectResourceType type, const QString& fileName) const;

    QString       m_rootDir;
    ProjectScreen m_screen;
};

LEKCO_END_NAMESPACE

#endif // !_PROJECT_HPP_
