/**
 * @file ResourceEditDialog.hpp
 * @brief Resource edit dialog for project assets.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2026-05-12
 * @license MIT
 */

#pragma once

#ifndef _RESOURCEEDITDIALOG_HPP_
#define _RESOURCEEDITDIALOG_HPP_

#include <QDialog>

#include "common/Common.h"
#include "project/Project.hpp"

class QLabel;
class QLineEdit;

LEKCO_BEGIN_NAMESPACE

class ResourceEditDialog
    : public QDialog
{
    Q_OBJECT

public:
    explicit ResourceEditDialog(ProjectResourceType type, const QString& fileName, QWidget* parent = nullptr);

    QString fileName() const;
    QString replacementPath() const;

private:
    void browseReplacement();

    ProjectResourceType m_type;
    QLineEdit*          m_fileNameEdit    = nullptr;
    QLineEdit*          m_replacementEdit = nullptr;
};

LEKCO_END_NAMESPACE

#endif // !_RESOURCEEDITDIALOG_HPP_
