/**
 * @file CanvasWorkspace.cpp
 * @brief Canvas preview workspace implementation for the tabula desktop client.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2026-05-13
 * @license MIT
 */

#include <QHBoxLayout>
#include <QMessageBox>
#include <QSizePolicy>
#include <QVBoxLayout>
#include <QWidget>

#include "controls/bars/RotationBar.hpp"
#include "controls/panels/ToolPanel.hpp"
#include "controls/widgets/CanvasPreviewer.hpp"
#include "controls/workspaces/CanvasWorkspace.hpp"
#include "project/BitmapProvider.hpp"
#include "project/FontProvider.hpp"

LEKCO_BEGIN_NAMESPACE

CanvasWorkspace::CanvasWorkspace(const epd_gfx_canvas_config_t& config, FontProvider* fontProvider,
    BitmapProvider* bitmapProvider, QWidget* parent)
    : QWidget(parent)
    , m_canvasConfig(config)
{
    auto* root = new QHBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto* leftPane = new QWidget(this);
    leftPane->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    auto* leftLayout = new QVBoxLayout(leftPane);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(0);

    m_previewer = new CanvasPreviewer(m_canvasConfig, this);
    m_previewer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    leftLayout->addWidget(m_previewer, 1);

    auto* rightPane = new QWidget(this);
    rightPane->setFixedWidth(kWorkspaceDetailsPaneWidth);
    rightPane->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    auto* rightLayout = new QVBoxLayout(rightPane);
    rightLayout->setContentsMargins(8, 10, 8, 10);
    rightLayout->setSpacing(12);

    m_rotationBar = new RotationBar(rightPane);
    m_rotationBar->setCurrentTool(m_canvasConfig.rotation);
    rightLayout->addWidget(m_rotationBar);
    connect(m_rotationBar, &RotationBar::rotationChanged, m_previewer, &CanvasPreviewer::setRotation);

    m_toolPanel = new ToolPanel(fontProvider, bitmapProvider, rightPane);
    m_toolPanel->updateCanvas(m_previewer->getCanvas());
    rightLayout->addWidget(m_toolPanel);
    rightLayout->addStretch(1);
    connect(m_toolPanel, &ToolPanel::refreshRequested, m_previewer, &CanvasPreviewer::refresh);
    connect(m_toolPanel, &ToolPanel::drawRequested, m_previewer, &CanvasPreviewer::drawCanvas);
    connect(m_toolPanel, &ToolPanel::previewRequested, m_previewer, &CanvasPreviewer::drawPreview);
    connect(m_previewer, &CanvasPreviewer::rotationChanged, this, [this]() {
        m_toolPanel->updateCanvas(m_previewer->getCanvas());
    });
    connect(m_previewer, &CanvasPreviewer::errorOccurred, this, [this](const QString& message) {
        QMessageBox::critical(this, QStringLiteral("Error"), message);
    });

    auto* divider = new QWidget(this);
    divider->setFixedWidth(1);
    divider->setStyleSheet(QStringLiteral("background-color: #b0b0b0;"));
    divider->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

    root->addWidget(leftPane, 1);
    root->addWidget(divider);
    root->addWidget(rightPane, 0);
    root->setStretch(0, 1);
    root->setStretch(1, 0);
    root->setStretch(2, 0);
}

void CanvasWorkspace::refreshProjectResources()
{
    if (m_toolPanel) {
        m_toolPanel->refreshProjectResources();
    }
}

LEKCO_END_NAMESPACE
