/**
 * @file ProjectDialog.hpp
 * @brief Project startup dialog for the tabula desktop client.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2026-05-12
 * @license MIT
 */

#pragma once

#ifndef _PROJECTDIALOG_HPP_
#define _PROJECTDIALOG_HPP_

#include <QDialog>

#include "common/Common.h"
#include "project/Project.hpp"

class QSpinBox;

LEKCO_BEGIN_NAMESPACE

class ProjectDialog
    : public QDialog
{
    Q_OBJECT

public:
    explicit ProjectDialog(QWidget* parent = nullptr);

    Project project() const;

private:
    void newProject();
    void openProject();
    bool showProjectError(const QString& error);

    Project m_project;
};

LEKCO_END_NAMESPACE

#endif // !_PROJECTDIALOG_HPP_
