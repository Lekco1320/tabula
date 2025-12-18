/**
 * @file ToolPanel.cpp
 * @brief Tool panel for MainWindow.
 * 
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2025-12-18
 * @license MIT
 */

#include <QVBoxLayout>

#include "controls/bars/ToolBar.hpp"
#include "controls/panels/LinePanel.hpp"
#include "controls/panels/ToolPanel.hpp"
#include "controls/widgets/CanvasPreviewer.hpp"
#include "controls/widgets/AdaptiveStackedWidget.hpp"

LEKCO_BEGIN_NAMESPACE

ToolPanel::ToolPanel(CanvasPreviewer* previewer, QWidget* parent)
    : QWidget(parent)
    , m_toolBar(new ToolBar(this))
    , m_stackedWidget(new AdaptiveStackedWidget(this))
{
    m_stackedWidget->setContentsMargins(0, 0, 0, 0);

    auto* hLinePanel = new LinePanel(QStringLiteral("Draw Horizontal Line"), Qt::Orientation::Horizontal, this);
    auto* vLinePanel = new LinePanel(QStringLiteral("Draw Vertical Line"), Qt::Orientation::Vertical, this);
    addControlPanel(hLinePanel);
    addControlPanel(vLinePanel);
    m_stackedWidget->setCollapsed(true);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);
    layout->addWidget(m_toolBar);
    layout->addWidget(m_stackedWidget);

    connect(m_toolBar, &ToolBar::toolChanged, this, [this](ToolBar::Tool tool) {
        int id = static_cast<int>(tool);
        if (id > -1 && id < 2) {
            m_stackedWidget->setCurrentIndex(id);
            m_stackedWidget->setCollapsed(false);
            m_controlPanels[id]->updatePreview();
        } else if (id == -1) {
            m_stackedWidget->setCollapsed(true);
        }
    });
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

void ToolPanel::addControlPanel(ControlPanel* panel)
{
    m_controlPanels.append(panel);
    m_stackedWidget->addWidget(panel);
    connect(panel, &ControlPanel::refreshRequested, this, [this]() {
        emit refreshRequested();
    });
    connect(panel, &ControlPanel::drawRequested, this, [this](DrawFunc func) {
        emit drawRequested(func);
    });
    connect(panel, &ControlPanel::previewRequested, this, [this](DrawFunc func) {
        emit previewRequested(func);
    });
}

LEKCO_END_NAMESPACE
