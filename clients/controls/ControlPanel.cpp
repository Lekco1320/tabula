/**
 * @file ControlPanel.cpp
 * @brief Base panel for canvas-bound controls.
 * 
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2025-12-13
 * @license MIT
 */

#include <QGridLayout>
#include <QCheckBox>
#include <QLabel>

#include "ControlPanel.hpp"
#include "CanvasPreviewer.hpp"

LEKCO_BEGIN_NAMESPACE

ControlPanel::ControlPanel(CanvasPreviewer* previewer, QWidget* parent)
    : QGroupBox(parent)
    , m_previewer(previewer)
    , m_root(new QGridLayout(this))
    , m_enablePreview(false)
{
    m_root->setContentsMargins(8, 8, 8, 8);
    m_root->setHorizontalSpacing(8);
    m_root->setVerticalSpacing(5);
}

void ControlPanel::flushToCanvas()
{
    epd_gfx_canvas_t canvas = m_previewer->getCanvas();
    flushTo(canvas);
    m_previewer->refresh();
}

void ControlPanel::flushToPreview()
{
    if (!m_enablePreview) {
        m_previewer->refresh();
        return;
    }

    epd_gfx_canvas_t canvas = m_previewer->getCanvas();
    epd_gfx_canvas_t cloned = nullptr;

    epd_err_t status = epd_gfx_canvas_clone(canvas, &cloned);
    if (status != EPD_OK) {
        throw std::runtime_error(QStringLiteral("Cannot clone canvas from previewer: %1")
            .arg(epd_err_to_str(status))
            .toStdString());
    }

    flushTo(cloned);
    m_previewer->setPreviewCanvas(cloned);
    epd_gfx_canvas_destroy(cloned);
}

LEKCO_END_NAMESPACE
