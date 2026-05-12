/**
 * @file MainWindow.cpp
 * @brief Main window implementation for the tabula desktop client.
 * 
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2025-12-9
 * @license MIT
 */

#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QIcon>
#include <QMessageBox>
#include <QPushButton>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QWidget>
#include <epd_gfx/canvas.h>
#include <oclero/qlementine/widgets/Label.hpp>

#include "controls/windows/MainWindow.hpp"
#include "controls/windows/PreviewWindow.hpp"
#include "controls/windows/ResourceEditDialog.hpp"

LEKCO_BEGIN_NAMESPACE

namespace {

constexpr int kResourceTypeRole = Qt::UserRole + 1;
constexpr int kResourceFileRole = Qt::UserRole + 2;

} // namespace

MainWindow::MainWindow(const Project& project, QWidget *parent)
    : QMainWindow(parent)
    , m_project(project)
{
    setFixedSize(QSize { 320, 520 });
    setWindowTitle(QFileInfo(m_project.rootDir()).fileName());

    auto* central = new QWidget(this);
    auto* layout  = new QVBoxLayout(central);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(8);

    auto* assetsLabel = new oclero::qlementine::Label(
        QStringLiteral("Assets"),
        oclero::qlementine::TextRole::H5,
        central);
    layout->addWidget(assetsLabel);

    m_resourceTree = new QTreeWidget(central);
    m_resourceTree->setHeaderHidden(true);
    layout->addWidget(m_resourceTree, 1);
    connect(m_resourceTree, &QTreeWidget::currentItemChanged, this, [this]() {
        updateResourceButtons();
    });

    auto* buttonRow    = new QWidget(central);
    auto* buttonLayout = new QHBoxLayout(buttonRow);
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->setSpacing(6);

    m_addButton    = new QPushButton(QStringLiteral("Add"), buttonRow);
    m_editButton   = new QPushButton(QStringLiteral("Edit"), buttonRow);
    m_deleteButton = new QPushButton(QStringLiteral("Delete"), buttonRow);
    buttonLayout->addWidget(m_addButton);
    buttonLayout->addWidget(m_editButton);
    buttonLayout->addWidget(m_deleteButton);
    layout->addWidget(buttonRow);

    m_previewButton = new QPushButton(QStringLiteral("Preview"), central);
    layout->addWidget(m_previewButton);

    connect(m_addButton, &QPushButton::clicked, this, &MainWindow::addSelectedResource);
    connect(m_editButton, &QPushButton::clicked, this, &MainWindow::editSelectedResource);
    connect(m_deleteButton, &QPushButton::clicked, this, &MainWindow::deleteSelectedResource);
    connect(m_previewButton, &QPushButton::clicked, this, &MainWindow::openPreviewWindow);

    setCentralWidget(central);
    refreshResourceTree();
}

void MainWindow::refreshResourceTree()
{
    m_resourceTree->clear();
    addResources(ProjectResourceType::Fonts, nullptr);

    for (int i = 0; i < m_resourceTree->topLevelItemCount(); ++i) {
        m_resourceTree->topLevelItem(i)->setExpanded(true);
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

void MainWindow::updateResourceButtons()
{
    const ProjectResourceType type = selectedResourceType();
    const bool hasType = type != ProjectResourceType::Unknown;
    const bool hasResource = selectedItemIsResource();
    m_addButton->setEnabled(hasType);
    m_editButton->setEnabled(hasResource);
    m_deleteButton->setEnabled(hasResource);
}

void MainWindow::addSelectedResource()
{
    const ProjectResourceType type = selectedResourceType();
    if (type == ProjectResourceType::Unknown) {
        return;
    }

    const QString file = QFileDialog::getOpenFileName(this, QStringLiteral("Add %1").arg(Project::displayName(type)),
        QString(), Project::fileDialogFilter(type));
    if (file.isEmpty()) {
        return;
    }

    QString error;
    if (!m_project.addResourceFromFile(type, file, &error)) {
        QMessageBox::critical(this, QStringLiteral("Asset Error"), error);
        return;
    }

    refreshResourceTree();
}

void MainWindow::editSelectedResource()
{
    const ProjectResourceType type = selectedResourceType();
    const QString fileName = selectedResourceFileName();
    if (type == ProjectResourceType::Unknown || fileName.isEmpty()) {
        return;
    }

    ResourceEditDialog dialog(type, fileName, this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    QString error;
    if (!m_project.updateResource(type, fileName, dialog.fileName(), dialog.replacementPath(), &error)) {
        QMessageBox::critical(this, QStringLiteral("Asset Error"), error);
        return;
    }

    refreshResourceTree();
}

void MainWindow::deleteSelectedResource()
{
    const ProjectResourceType type = selectedResourceType();
    const QString fileName = selectedResourceFileName();
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

void MainWindow::openPreviewWindow()
{
    if (!m_previewWindow) {
        const ProjectScreen screen = m_project.screen();

        epd_gfx_canvas_config_t config;
        config.width    = static_cast<uint16_t>(screen.width);
        config.height   = static_cast<uint16_t>(screen.height);
        config.format   = EPD_GFX_FORMAT_NATIVE;
        config.rotation = EPD_GFX_ROTATE_0;

        m_previewWindow = new PreviewWindow(config, this);
        connect(m_previewWindow, &QObject::destroyed, this, [this]() {
            m_previewWindow = nullptr;
        });
    }

    m_previewWindow->show();
    m_previewWindow->raise();
    m_previewWindow->activateWindow();
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
