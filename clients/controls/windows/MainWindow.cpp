/**
 * @file MainWindow.cpp
 * @brief Main window implementation for the tabula desktop client.
 * 
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2025-12-9
 * @license MIT
 */

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QSizePolicy>
#include <QVariant>
#include <QFrame>
#include <QScreen>

#include "controls/panels/ToolPanel.hpp"
#include "controls/bars/CursorBar.hpp"
#include "controls/widgets/CanvasPreviewer.hpp"
#include "controls/windows/MainWindow.hpp"

LEKCO_BEGIN_NAMESPACE

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setMinimumSize(QSize { 1050, 820 });

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

    epd_gfx_canvas_config_t cfg {
        .width    = 640,
        .height   = 384,
        .format   = EPD_GFX_FORMAT_NATIVE,
        .rotation = EPD_GFX_ROTATE_0,
    };
    m_previewer = new CanvasPreviewer(cfg, central);
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
    connect(m_cursorBar, &CursorBar::cursorChanged, this, [this](CursorBar::Cursor cursor) {
        m_previewer->setCursor(cursor);
    });

    // Toolbar
    m_toolPanel = new ToolPanel(m_previewer, rightPane);
    m_toolPanel->setContentsMargins(0, 0, 0, 0);
    m_toolPanel->updateCanvas(m_previewer->getCanvas());
    rightLayout->addWidget(m_toolPanel);
    connect(m_toolPanel, &ToolPanel::refreshRequested, this, [this]() {
        m_previewer->refresh();
    });
    connect(m_toolPanel, &ToolPanel::drawRequested, this, [this](DrawFunc func) {
        m_previewer->drawCanvas(func);
    });
    connect(m_toolPanel, &ToolPanel::previewRequested, this, [this](DrawFunc func) {
        m_previewer->drawPreview(func);
    });
    connect(m_previewer, &CanvasPreviewer::rotationChanged, this, [this]() {
        m_toolPanel->updateCanvas(m_previewer->getCanvas());
    });

    // Rotation comboBox
    m_rotationCombo = new QComboBox(rightPane);
    m_rotationCombo->addItem(QStringLiteral("0°"), QVariant::fromValue<int>(EPD_GFX_ROTATE_0));
    m_rotationCombo->addItem(QStringLiteral("90°"), QVariant::fromValue<int>(EPD_GFX_ROTATE_90));
    m_rotationCombo->addItem(QStringLiteral("180°"), QVariant::fromValue<int>(EPD_GFX_ROTATE_180));
    m_rotationCombo->addItem(QStringLiteral("270°"), QVariant::fromValue<int>(EPD_GFX_ROTATE_270));
    m_rotationCombo->setCurrentIndex(0);
    connect(m_rotationCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, [this, cfg]() {
            auto rot = static_cast<epd_gfx_rotation_t>(m_rotationCombo->currentData().toInt());
            m_previewer->setRotation(rot);
        });

    // Draw button
    m_drawButton = new QPushButton(QStringLiteral("Draw!"), rightPane);
    connect(m_drawButton, &QPushButton::clicked, this, [this]() { drawDemo(); });

    // Rotation setting pane
    auto* rotationPane = new QWidget(rightPane);
    auto* rotationLayout = new QHBoxLayout(rotationPane);
    rotationLayout->setContentsMargins(0, 0, 0, 0);
    rotationLayout->setSpacing(10);
    rotationLayout->addWidget(new QLabel(QStringLiteral("Rotation: "), rightPane));
    rotationLayout->addWidget(m_rotationCombo, 1);

    // Add widgets to right pane
    rightLayout->addWidget(rotationPane);
    rightLayout->addWidget(m_drawButton);
    rightLayout->addStretch(1);

    // Middile divider
    auto* divider = new QFrame(central);
    divider->setFrameShape(QFrame::VLine);
    divider->setFrameShadow(QFrame::Plain);
    divider->setLineWidth(1);
    divider->setMidLineWidth(0);
    divider->setFixedWidth(1);
    divider->setStyleSheet("color: #b0b0b0;");
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

MainWindow::~MainWindow()
{
}

void MainWindow::drawDemo()
{
    auto canvas = m_previewer->getCanvas();
    epd_gfx_canvas_fill(canvas, EPD_GFX_WHITE);
    epd_gfx_canvas_draw_hline(canvas, 200, 150, 200, EPD_GFX_BLACK);
    epd_gfx_canvas_draw_vline(canvas, 100, 50, 200, EPD_GFX_RED);
    epd_gfx_canvas_draw_rect(canvas, 295, 50, 100, 150, EPD_GFX_BLACK);
    epd_gfx_canvas_fill_rect(canvas, 296, 51, 98, 148, EPD_GFX_RED);
    m_previewer->refresh();
}

LEKCO_END_NAMESPACE
