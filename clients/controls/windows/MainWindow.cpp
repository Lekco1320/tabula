/**
 * @file MainWindow.cpp
 * @brief Main window implementation for the tabula desktop client.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2025-12-9
 * @license MIT
 */

#include <QFileInfo>
#include <QHBoxLayout>
#include <QIcon>
#include <QMessageBox>
#include <QPushButton>
#include <QSizePolicy>
#include <QStackedWidget>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QWidget>
#include <oclero/qlementine/widgets/Label.hpp>
#include <epd_gfx/canvas.h>

#include "controls/windows/MainWindow.hpp"
#include "controls/windows/NewFontDialog.hpp"
#include "controls/workspaces/CanvasWorkspace.hpp"
#include "controls/workspaces/FontWorkspace.hpp"

LEKCO_BEGIN_NAMESPACE

BEGIN_NAMESPACE()

constexpr int kResourceTypeRole = Qt::UserRole + 1;
constexpr int kResourceFileRole = Qt::UserRole + 2;
constexpr int kAssetsPaneWidth  = 240;
constexpr int kWorkspaceMinW    = 1050;
constexpr int kWorkspaceMinH    = 750;

END_NAMESPACE

MainWindow::MainWindow(const Project& project, QWidget* parent)
    : QMainWindow(parent)
    , m_project(project)
    , m_fontProvider(&m_project)
{
    setMinimumSize(QSize { kAssetsPaneWidth + kWorkspaceMinW, kWorkspaceMinH });
    setWindowTitle(QFileInfo(m_project.rootDir()).fileName());

    auto* central = new QWidget(this);
    auto* root    = new QHBoxLayout(central);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto* assetsPane = new QWidget(central);
    assetsPane->setFixedWidth(kAssetsPaneWidth);
    assetsPane->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    auto* assetsLayout = new QVBoxLayout(assetsPane);
    assetsLayout->setContentsMargins(10, 10, 10, 10);
    assetsLayout->setSpacing(8);

    auto* assetsLabel = new oclero::qlementine::Label(
        QStringLiteral("Assets"),
        oclero::qlementine::TextRole::H5,
        assetsPane);
    assetsLayout->addWidget(assetsLabel);

    m_resourceTree = new QTreeWidget(assetsPane);
    m_resourceTree->setHeaderHidden(true);
    assetsLayout->addWidget(m_resourceTree, 1);
    connect(m_resourceTree, &QTreeWidget::currentItemChanged, this, [this]() {
        updateWorkspace();
        updateResourceButtons();
    });

    auto* buttonRow    = new QWidget(assetsPane);
    auto* buttonLayout = new QHBoxLayout(buttonRow);
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->setSpacing(6);

    m_addButton    = new QPushButton(QStringLiteral("Add"), buttonRow);
    m_deleteButton = new QPushButton(QStringLiteral("Delete"), buttonRow);
    buttonLayout->addWidget(m_addButton);
    buttonLayout->addWidget(m_deleteButton);
    assetsLayout->addWidget(buttonRow);

    const ProjectScreen screen = m_project.screen();
    epd_gfx_canvas_config_t config;
    config.width    = static_cast<uint16_t>(screen.width);
    config.height   = static_cast<uint16_t>(screen.height);
    config.format   = EPD_GFX_FORMAT_NATIVE;
    config.rotation = EPD_GFX_ROTATE_0;

    m_workspaceStack  = new QStackedWidget(central);
    m_canvasWorkspace = new CanvasWorkspace(config, &m_fontProvider, m_workspaceStack);
    m_fontWorkspace   = new FontWorkspace(m_workspaceStack);
    m_workspaceStack->addWidget(m_canvasWorkspace);
    m_workspaceStack->addWidget(m_fontWorkspace);
    m_workspaceStack->setCurrentWidget(m_canvasWorkspace);

    connect(m_addButton, &QPushButton::clicked, this, &MainWindow::addSelectedResource);
    connect(m_deleteButton, &QPushButton::clicked, this, &MainWindow::deleteSelectedResource);

    root->addWidget(assetsPane);
    root->addWidget(m_workspaceStack, 1);

    setCentralWidget(central);
    refreshResourceTree();
}

void MainWindow::refreshResourceTree()
{
    m_resourceTree->clear();

    auto* canvasItem = new QTreeWidgetItem(m_resourceTree);
    canvasItem->setText(0, QStringLiteral("Canvas Preview"));
    canvasItem->setIcon(0, QIcon(QStringLiteral(":/common/icons/Pointer.svg")));
    canvasItem->setData(0, kResourceTypeRole, static_cast<int>(ProjectResourceType::Unknown));

    addResources(ProjectResourceType::Fonts, nullptr);

    for (int i = 0; i < m_resourceTree->topLevelItemCount(); ++i) {
        m_resourceTree->topLevelItem(i)->setExpanded(true);
    }
    if (m_canvasWorkspace) {
        m_canvasWorkspace->refreshFonts();
    }
    m_resourceTree->setCurrentItem(m_resourceTree->topLevelItem(0));
    updateResourceButtons();
}

void MainWindow::addResources(ProjectResourceType type, QTreeWidgetItem* parentItem)
{
    auto* category = parentItem ? new QTreeWidgetItem(parentItem) : new QTreeWidgetItem(m_resourceTree);
    category->setText(0, Project::displayName(type));
    category->setIcon(0, QIcon(QStringLiteral(":/common/icons/Font.svg")));
    category->setData(0, kResourceTypeRole, static_cast<int>(type));

    const QVector<ProjectResource> resources = m_project.resources(type);
    for (const ProjectResource& resource : resources) {
        auto* item = new QTreeWidgetItem(category);
        item->setText(0, resource.fileName);
        item->setIcon(0, QIcon(QStringLiteral(":/common/icons/FontFile.svg")));
        item->setData(0, kResourceTypeRole, static_cast<int>(resource.type));
        item->setData(0, kResourceFileRole, resource.fileName);
    }
}

void MainWindow::selectResource(ProjectResourceType type, const QString& fileName)
{
    for (int i = 0; i < m_resourceTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem* category = m_resourceTree->topLevelItem(i);
        if (static_cast<ProjectResourceType>(category->data(0, kResourceTypeRole).toInt()) != type) {
            continue;
        }

        for (int j = 0; j < category->childCount(); ++j) {
            QTreeWidgetItem* item = category->child(j);
            if (item->data(0, kResourceFileRole).toString() == fileName) {
                m_resourceTree->setCurrentItem(item);
                return;
            }
        }
    }
}

void MainWindow::updateWorkspace()
{
    const ProjectResourceType type     = selectedResourceType();
    const QString             fileName = selectedResourceFileName();
    if (type == ProjectResourceType::Fonts) {
        const QVector<ProjectResource> resources = m_project.resources(type);
        if (fileName.isEmpty()) {
            m_fontWorkspace->setFontResources(resources);
            m_workspaceStack->setCurrentWidget(m_fontWorkspace);
            return;
        }

        for (const ProjectResource& resource : resources) {
            if (resource.fileName == fileName) {
                if (m_fontWorkspace->resourcePath() != resource.absolutePath) {
                    m_fontWorkspace->setResource(resource);
                }
                m_workspaceStack->setCurrentWidget(m_fontWorkspace);
                return;
            }
        }
    }

    m_fontWorkspace->clearResource();
    m_workspaceStack->setCurrentWidget(m_canvasWorkspace);
}

void MainWindow::updateResourceButtons()
{
    const ProjectResourceType type = selectedResourceType();
    const bool                hasType     = type != ProjectResourceType::Unknown;
    const bool                hasResource = selectedItemIsResource();
    m_addButton->setEnabled(hasType);
    m_deleteButton->setEnabled(hasResource);
}

void MainWindow::addSelectedResource()
{
    const ProjectResourceType type = selectedResourceType();
    if (type == ProjectResourceType::Unknown) {
        return;
    }

    if (type == ProjectResourceType::Fonts) {
        NewFontDialog dialog(this);
        if (dialog.exec() != QDialog::Accepted) {
            return;
        }

        QString fileName;
        QString error;
        if (!m_project.createFontResource(dialog.fontName(), &fileName, &error)) {
            QMessageBox::critical(this, QStringLiteral("Font Error"), error);
            return;
        }

        refreshResourceTree();
        selectResource(type, fileName);
        updateResourceButtons();
        return;
    }
}

void MainWindow::deleteSelectedResource()
{
    const ProjectResourceType type     = selectedResourceType();
    const QString             fileName = selectedResourceFileName();
    if (type == ProjectResourceType::Unknown || fileName.isEmpty()) {
        return;
    }

    const int result = QMessageBox::question(this, QStringLiteral("Delete Asset"),
        QStringLiteral("Delete %1?").arg(fileName));
    if (result != QMessageBox::Yes) {
        return;
    }

    QString error;
    if (!m_project.removeResource(type, fileName, &error)) {
        QMessageBox::critical(this, QStringLiteral("Asset Error"), error);
        return;
    }

    refreshResourceTree();
}

ProjectResourceType MainWindow::selectedResourceType() const
{
    QTreeWidgetItem* item = m_resourceTree->currentItem();
    if (!item) {
        return ProjectResourceType::Unknown;
    }

    const int value = item->data(0, kResourceTypeRole).toInt();
    return static_cast<ProjectResourceType>(value);
}

QString MainWindow::selectedResourceFileName() const
{
    QTreeWidgetItem* item = m_resourceTree->currentItem();
    return item ? item->data(0, kResourceFileRole).toString() : QString();
}

bool MainWindow::selectedItemIsResource() const
{
    return !selectedResourceFileName().isEmpty();
}

LEKCO_END_NAMESPACE
