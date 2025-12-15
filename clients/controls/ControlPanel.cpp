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
    m_root->setContentsMargins(6, 6, 6, 6);
    m_root->setSpacing(4);
}

void ControlPanel::refreshPreview()
{
    epd_gfx_canvas_t canvas = m_previewer->getCanvas();
    epd_gfx_canvas_t cloned = nullptr;

    epd_err_t status = epd_gfx_canvas_clone(canvas, &cloned);
    if (status != EPD_OK) {
        throw std::runtime_error(QStringLiteral("Cannot clone canvas from previewer: %1")
            .arg(epd_err_to_str(status))
            .toStdString());
    }

    flushToCanvas(canvas);
}

QWidget* ControlPanel::makeRow(const QString& label, QWidget* editor) const
{
    auto* w      = new QWidget(const_cast<ControlPanel*>(this));
    auto* layout = new QHBoxLayout(w);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(new QLabel(label, w));
    layout->addWidget(editor, 1);
    return w;
}

LEKCO_END_NAMESPACE
