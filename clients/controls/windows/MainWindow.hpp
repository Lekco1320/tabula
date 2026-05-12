/**
 * @file MainWindow.hpp
 * @brief Main window for the tabula desktop client.
 * 
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2025-12-9
 * @license MIT
 */

#pragma once

#ifndef _MAINWINDOW_H_
#define _MAINWINDOW_H_

#include <QMainWindow>

#include "common/Common.h"
#include "project/Project.hpp"

class QPushButton;
class QTreeWidget;
class QTreeWidgetItem;

LEKCO_BEGIN_NAMESPACE

class PreviewWindow;

class MainWindow
    : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(const Project& project, QWidget* parent = nullptr);

private:
    void refreshResourceTree();
    void addResources(ProjectResourceType type, QTreeWidgetItem* parentItem);
    void updateResourceButtons();
    void addSelectedResource();
    void editSelectedResource();
    void deleteSelectedResource();
    void openPreviewWindow();
    ProjectResourceType selectedResourceType() const;
    QString selectedResourceFileName() const;
    bool selectedItemIsResource() const;

    Project        m_project;
    QTreeWidget*   m_resourceTree  = nullptr;
    QPushButton*   m_addButton     = nullptr;
    QPushButton*   m_editButton    = nullptr;
    QPushButton*   m_deleteButton  = nullptr;
    QPushButton*   m_previewButton = nullptr;
    PreviewWindow* m_previewWindow = nullptr;
};

LEKCO_END_NAMESPACE

#endif // !_MAINWINDOW_H_
