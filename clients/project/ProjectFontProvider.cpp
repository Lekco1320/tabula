/**
 * @file ProjectFontProvider.cpp
 * @brief Project-backed font resource provider implementation.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2026-05-16
 * @license MIT
 */

#include <QFile>
#include <QFileInfo>
#include <epd_gfx/egf.h>

#include "project/EpdStreamAdapter.hpp"
#include "project/ProjectFontProvider.hpp"

LEKCO_BEGIN_NAMESPACE

ProjectFontProvider::ProjectFontProvider(const Project* project)
    : m_project(project)
{
}

void ProjectFontProvider::setProject(const Project* project)
{
    m_project = project;
}

QVector<FontResourceInfo> ProjectFontProvider::fonts() const
{
    QVector<FontResourceInfo> result;
    if (!m_project) {
        return result;
    }

    const QVector<ProjectResource> resources = m_project->resources(ProjectResourceType::Fonts);
    for (const ProjectResource& resource : resources) {
        FontResourceInfo info;
        info.fileName     = resource.fileName;
        info.displayName  = QFileInfo(resource.fileName).completeBaseName();
        info.absolutePath = resource.absolutePath;
        result.append(info);
    }
    return result;
}

QVector<uint16_t> ProjectFontProvider::sizes(const QString& fileName) const
{
    QVector<uint16_t> result;

    FontResourceInfo info;
    if (!font(fileName, &info)) {
        return result;
    }

    QFile file(info.absolutePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return result;
    }

    EpdStreamAdapter stream(&file);
    epd_gfx_egf_header_t header = {};
    if (!epd_gfx_egf_read_header(stream.stream(), &header) || !epd_gfx_egf_check_magic(&header)) {
        return result;
    }

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

bool ProjectFontProvider::font(const QString& fileName, FontResourceInfo* outInfo) const
{
    if (!m_project || fileName.isEmpty()) {
        return false;
    }

    const QVector<FontResourceInfo> fontList = fonts();
    for (const FontResourceInfo& info : fontList) {
        if (info.fileName == fileName) {
            if (outInfo) {
                *outInfo = info;
            }
            return true;
        }
    }
    return false;
}

LEKCO_END_NAMESPACE
