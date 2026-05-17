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
#include <QPushButton>
#include <QSpinBox>

#include "controls/panels/ControlPanel.hpp"

LEKCO_BEGIN_NAMESPACE

ControlPanel::ControlPanel(const QString& title, QWidget* parent)
    : QGroupBox(title, parent)
    , m_root(new QGridLayout(this))
    , m_enablePreview(false)
{
    m_root->setContentsMargins(8, 8, 8, 8);
    m_root->setHorizontalSpacing(10);
    m_root->setVerticalSpacing(5);
}

void ControlPanel::updateDraw() const
{
    auto func = drawFunc();
    emit drawRequested(func);
}

void ControlPanel::updatePreview() const
{
    auto func = drawFunc();
    if (m_enablePreview) {
        emit previewRequested(func);
    } else {
        emit refreshRequested();
    }
}

void ControlPanel::updateRange(epd_gfx_canvas_t canvas)
{
}

void ControlPanel::refreshProjectResources()
{
}

QCheckBox* ControlPanel::createPreviewCheckBox()
{
    auto* preview = new QCheckBox(QStringLiteral("Preview"), this);
    preview->setStyleSheet(QStringLiteral("QCheckBox { spacing: 4px; }"));
    connect(preview, &QCheckBox::checkStateChanged, [this](int checked) {
        m_enablePreview = (bool)checked;
        updatePreview();
    });
    return preview;
}

QPushButton* ControlPanel::createDrawButton(const QString& text)
{
    auto* draw = new QPushButton(text, this);
    connect(draw, &QPushButton::clicked, this, &ControlPanel::updateDraw);
    return draw;
}

void ControlPanel::setPointRange(QSpinBox* x, QSpinBox* y, epd_gfx_canvas_t canvas) const
{
    const uint16_t width  = epd_gfx_canvas_get_logical_width(canvas);
    const uint16_t height = epd_gfx_canvas_get_logical_height(canvas);

    x->setRange(1, width);
    y->setRange(1, height);
}

LEKCO_END_NAMESPACE
