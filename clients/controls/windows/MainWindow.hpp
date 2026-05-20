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
#include <QString>

#include "common/Common.h"
#include "project/FontProvider.hpp"
#include "project/Project.hpp"

class QPushButton;
class QStackedWidget;
class QTreeWidget;
class QTreeWidgetItem;

LEKCO_BEGIN_NAMESPACE

class BitmapWorkspace;
class CanvasWorkspace;
class FontWorkspace;

class MainWindow
    : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(const Project& project, QWidget* parent = nullptr);

private:
    void refreshResourceTree();
    void addResources(ProjectResourceType type, QTreeWidgetItem* parentItem);
    void selectResource(ProjectResourceType type, const QString& fileName);
    void updateWorkspace();
    void updateResourceButtons();
    void addSelectedResource();
    void deleteSelectedResource();
    void exportAssets();
    ProjectResourceType selectedResourceType() const;
    QString selectedResourceFileName() const;
    bool selectedItemIsResource() const;

    Project             m_project;
    FontProvider        m_fontProvider;
    QTreeWidget*        m_resourceTree     = nullptr;
    QPushButton*        m_addButton        = nullptr;
    QPushButton*        m_deleteButton     = nullptr;
    QPushButton*        m_exportButton     = nullptr;
    QStackedWidget*     m_workspaceStack   = nullptr;
    BitmapWorkspace*    m_bitmapWorkspace  = nullptr;
    CanvasWorkspace*    m_canvasWorkspace  = nullptr;
    FontWorkspace*      m_fontWorkspace    = nullptr;
};

LEKCO_END_NAMESPACE

#endif // !_MAINWINDOW_H_
