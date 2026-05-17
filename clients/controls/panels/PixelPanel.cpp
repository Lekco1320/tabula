/**
 * @file PixelPanel.cpp
 * @brief Panel to configure and draw a pixel.
 * 
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2025-12-21
 * @license MIT
 */

#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>

#include "controls/panels/PixelPanel.hpp"
#include "controls/widgets/ColorButton.hpp"

LEKCO_BEGIN_NAMESPACE

PixelPanel::PixelPanel(const QString& title, QWidget* parent)
    : ControlPanel(title, parent)
    , m_x(new QSpinBox(this))
    , m_y(new QSpinBox(this))
    , m_previewBtn(createPreviewCheckBox())
    , m_colorBtn(new ColorButton(this))
    , m_draw(createDrawButton())
{
    m_root->addWidget(MakeLabeledWidget(this, QStringLiteral("X:"), m_x), 0, 0);
    m_root->addWidget(MakeLabeledWidget(this, QStringLiteral("Y:"), m_y), 0, 1);
    m_root->addWidget(MakeRow(this, 0, m_previewBtn, m_colorBtn), 1, 0);
    m_root->addWidget(m_draw, 1, 1);

    connect(m_x, &QSpinBox::valueChanged, this, &PixelPanel::updatePreview);
    connect(m_y, &QSpinBox::valueChanged, this, &PixelPanel::updatePreview);
    connect(m_colorBtn, &ColorButton::colorChanged, this, &PixelPanel::updatePreview);
}

void PixelPanel::updateRange(const epd_gfx_canvas_t canvas)
{
    setPointRange(m_x, m_y, canvas);
}

DrawFunc PixelPanel::drawFunc() const
{
    const int x      = m_x->value();
    const int y      = m_y->value();
    const auto color = m_colorBtn->currentColor();
    return [x, y, color](epd_gfx_canvas_t c) {
        return epd_gfx_canvas_draw_pixel(c, epd_gfx_point_t{
            static_cast<uint16_t>(x),
            static_cast<uint16_t>(y),
        }, color);
    };
}

LEKCO_END_NAMESPACE
