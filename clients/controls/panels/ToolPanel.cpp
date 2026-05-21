/**
 * @file ToolPanel.cpp
 * @brief Tool panel for MainWindow.
 * 
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2025-12-18
 * @license MIT
 */

#include <QVBoxLayout>

#include "controls/panels/ToolPanel.hpp"
#include "controls/panels/LinePanel.hpp"
#include "controls/panels/RectPanel.hpp"
#include "controls/panels/FillPanel.hpp"
#include "controls/panels/BitmapPanel.hpp"
#include "controls/panels/PixelPanel.hpp"
#include "controls/panels/TextPanel.hpp"
#include "controls/widgets/AdaptiveStackedWidget.hpp"
#include "project/BitmapProvider.hpp"
#include "project/FontProvider.hpp"

LEKCO_BEGIN_NAMESPACE

ToolPanel::ToolPanel(FontProvider* fontProvider, BitmapProvider* bitmapProvider, QWidget* parent)
    : QWidget(parent)
    , m_toolBar(new ToolBar(this))
    , m_stackedWidget(new AdaptiveStackedWidget(this))
    , m_bitmapProvider(bitmapProvider)
    , m_fontProvider(fontProvider)
{
    m_stackedWidget->setContentsMargins(0, 0, 0, 0);

    auto* hLinePanel = new LinePanel(QStringLiteral("Draw Horizontal Line"), Qt::Orientation::Horizontal, this);
    auto* vLinePanel = new LinePanel(QStringLiteral("Draw Vertical Line"), Qt::Orientation::Vertical, this);
    auto* dRectPanel = new RectPanel(QStringLiteral("Draw Rectange"), true, this);
    auto* fRectPanel = new RectPanel(QStringLiteral("Fill Rectange"), false, this);
    auto* pixelPanel = new PixelPanel(QStringLiteral("Draw Pixel"), this);
    auto* fillPanel   = new FillPanel(QStringLiteral("Fill Panel"), this);
    auto* textPanel   = new TextPanel(QStringLiteral("Draw Text"), fontProvider, this);
    auto* bitmapPanel = new BitmapPanel(QStringLiteral("Draw Bitmap"), bitmapProvider, this);
    addControlPanel(hLinePanel);
    addControlPanel(vLinePanel);
    addControlPanel(dRectPanel);
    addControlPanel(fRectPanel);
    addControlPanel(pixelPanel);
    addControlPanel(fillPanel);
    addControlPanel(textPanel);
    addControlPanel(bitmapPanel);
    m_stackedWidget->setCollapsed(true);
    m_toolBar->setToolEnabled(ToolBar::Tool::DrawText,
        m_fontProvider && m_fontProvider->hasUsableFonts());
    m_toolBar->setToolEnabled(ToolBar::Tool::DrawBitmap,
        m_bitmapProvider && m_bitmapProvider->hasUsableBitmaps());

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);
    layout->addWidget(m_toolBar);
    layout->addWidget(m_stackedWidget);

    connect(m_toolBar, &ToolBar::toolChanged, this, &ToolPanel::setTool);
}

void ToolPanel::updateCanvas(const epd_gfx_canvas_t canvas)
{
    for (auto panel : m_controlPanels) {
        panel->updateRange(canvas);
        if (!m_stackedWidget->isCollapsed() && m_stackedWidget->currentWidget() == panel) {
            panel->updatePreview();
        }
    }
}

void ToolPanel::refreshProjectResources()
{
    for (auto panel : m_controlPanels) {
        panel->refreshProjectResources();
    }
    m_toolBar->setToolEnabled(ToolBar::Tool::DrawText,
        m_fontProvider && m_fontProvider->hasUsableFonts());
    m_toolBar->setToolEnabled(ToolBar::Tool::DrawBitmap,
        m_bitmapProvider && m_bitmapProvider->hasUsableBitmaps());
}

void ToolPanel::addControlPanel(ControlPanel* panel)
{
    m_controlPanels.append(panel);
    m_stackedWidget->addWidget(panel);
    connect(panel, &ControlPanel::refreshRequested, this, &ToolPanel::refreshRequested);
    connect(panel, &ControlPanel::drawRequested, this, &ToolPanel::drawRequested);
    connect(panel, &ControlPanel::previewRequested, this, &ToolPanel::previewRequested);
}

void ToolPanel::setTool(ToolBar::Tool tool)
{
    if (tool == ToolBar::Tool::None) {
        m_stackedWidget->setCollapsed(true);
        return;
    }

    int id = static_cast<int>(tool);
    if (id > -1 && id < m_controlPanels.size()) {
        m_stackedWidget->setCurrentIndex(id);
        m_stackedWidget->setCollapsed(false);
        m_controlPanels[id]->updatePreview();
    }
}

LEKCO_END_NAMESPACE
