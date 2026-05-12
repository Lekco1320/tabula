/**
 * @file PreviewWindow.cpp
 * @brief Canvas preview window implementation for the tabula desktop client.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2026-05-12
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
#include "controls/windows/PreviewWindow.hpp"

LEKCO_BEGIN_NAMESPACE

PreviewWindow::PreviewWindow(const epd_gfx_canvas_config_t& config, QWidget* parent)
    : QMainWindow(parent)
    , m_canvasConfig(config)
{
    setMinimumSize(QSize { 1050, 750 });

    auto* central = new QWidget(this);
    auto* root    = new QHBoxLayout(central);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // Left: Previewer pane
    auto* leftPane = new QWidget(central);
    leftPane->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    auto* leftLayout = new QVBoxLayout(leftPane);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(0);

    m_previewer = new CanvasPreviewer(m_canvasConfig, central);
    m_previewer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    leftLayout->addWidget(m_previewer, 1);

    // Right: Control pane
    auto* rightPane = new QWidget(central);
    rightPane->setFixedWidth(250);
    rightPane->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    auto* rightLayout = new QVBoxLayout(rightPane);
    rightLayout->setContentsMargins(10, 10, 12, 10);
    rightLayout->setSpacing(12);

    // Cursor Bar
    m_cursorBar = new CursorBar(rightPane);
    rightLayout->addWidget(m_cursorBar);
    connect(m_cursorBar, &CursorBar::cursorChanged, m_previewer, &CanvasPreviewer::setCursor);

    // Rotation comboBox
    m_rotationBar = new RotationBar(rightPane);
    m_rotationBar->setCurrentTool(m_canvasConfig.rotation);
    rightLayout->addWidget(m_rotationBar);
    connect(m_rotationBar, &RotationBar::rotationChanged, this, [this](epd_gfx_rotation_t rotation) {
        m_previewer->setRotation(rotation);
    });

    // Toolbar
    m_toolPanel = new ToolPanel(m_previewer, rightPane);
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

    // Middile divider
    auto* divider = new QFrame(central);
    divider->setFrameShape(QFrame::VLine);
    divider->setFrameShadow(QFrame::Plain);
    divider->setLineWidth(1);
    divider->setMidLineWidth(0);
    divider->setFixedWidth(1);
    divider->setStyleSheet(QStringLiteral("color: #b0b0b0;"));
    divider->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

    // Add all widgets to root layout
    root->addWidget(leftPane, 1);
    root->addWidget(divider);
    root->addWidget(rightPane, 0);
    root->setStretch(0, 1);
    root->setStretch(1, 0);
    root->setStretch(2, 0);

    setCentralWidget(central);
}

LEKCO_END_NAMESPACE
