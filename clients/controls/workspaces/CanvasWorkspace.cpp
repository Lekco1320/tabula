/**
 * @file CanvasWorkspace.cpp
 * @brief Canvas preview workspace implementation for the tabula desktop client.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2026-05-13
 * @license MIT
 */

#include <QFrame>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QSizePolicy>
#include <QVBoxLayout>
#include <QWidget>

#include "controls/bars/CursorBar.hpp"
#include "controls/bars/RotationBar.hpp"
#include "controls/panels/ToolPanel.hpp"
#include "controls/widgets/CanvasPreviewer.hpp"
#include "controls/workspaces/CanvasWorkspace.hpp"
#include "project/FontProvider.hpp"

LEKCO_BEGIN_NAMESPACE

CanvasWorkspace::CanvasWorkspace(const epd_gfx_canvas_config_t& config, FontProvider* fontProvider,
    QWidget* parent)
    : ResourceWorkspace(parent)
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
    rightPane->setFixedWidth(250);
    rightPane->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    auto* rightLayout = new QVBoxLayout(rightPane);
    rightLayout->setContentsMargins(10, 10, 12, 10);
    rightLayout->setSpacing(12);

    m_cursorBar = new CursorBar(rightPane);
    rightLayout->addWidget(m_cursorBar);
    connect(m_cursorBar, &CursorBar::cursorChanged, m_previewer, &CanvasPreviewer::setCursor);

    m_rotationBar = new RotationBar(rightPane);
    m_rotationBar->setCurrentTool(m_canvasConfig.rotation);
    rightLayout->addWidget(m_rotationBar);
    connect(m_rotationBar, &RotationBar::rotationChanged, this, [this](epd_gfx_rotation_t rotation) {
        m_previewer->setRotation(rotation);
    });

    m_toolPanel = new ToolPanel(fontProvider, rightPane);
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

    auto* divider = new QFrame(this);
    divider->setFrameShape(QFrame::VLine);
    divider->setFrameShadow(QFrame::Plain);
    divider->setLineWidth(1);
    divider->setMidLineWidth(0);
    divider->setFixedWidth(1);
    divider->setStyleSheet(QStringLiteral("color: #b0b0b0;"));
    divider->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

    root->addWidget(leftPane, 1);
    root->addWidget(divider);
    root->addWidget(rightPane, 0);
    root->setStretch(0, 1);
    root->setStretch(1, 0);
    root->setStretch(2, 0);
}

void CanvasWorkspace::setResource(const ProjectResource& resource)
{
    Q_UNUSED(resource)
}

void CanvasWorkspace::clearResource()
{
}

void CanvasWorkspace::refreshFonts()
{
    if (m_toolPanel) {
        m_toolPanel->refreshFonts();
    }
}

LEKCO_END_NAMESPACE
