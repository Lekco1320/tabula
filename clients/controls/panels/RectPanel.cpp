/**
 * @file RectPanel.cpp
 * @brief Panel to draw or fill a rectangle.
 * 
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2025-12-19
 * @license MIT
 */

#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>

#include "controls/panels/RectPanel.hpp"
#include "controls/widgets/ColorButton.hpp"

LEKCO_BEGIN_NAMESPACE

RectPanel::RectPanel(const QString& title, bool isDraw, QWidget* parent)
    : ControlPanel(title, parent)
    , m_isDraw(isDraw)
    , m_x(new QSpinBox(this))
    , m_y(new QSpinBox(this))
    , m_width(new QSpinBox(this))
    , m_height(new QSpinBox(this))
    , m_previewBtn(createPreviewCheckBox())
    , m_colorBtn(new ColorButton(this))
    , m_draw(createDrawButton())
{
    m_root->addWidget(MakeLabeledWidget(this, QStringLiteral("X:"), m_x), 0, 0);
    m_root->addWidget(MakeLabeledWidget(this, QStringLiteral("Y:"), m_y), 0, 1);
    m_root->addWidget(MakeLabeledWidget(this, QStringLiteral("W:"), m_width), 1, 0);
    m_root->addWidget(MakeLabeledWidget(this, QStringLiteral("H:"), m_height), 1, 1);
    m_root->addWidget(MakeRow(this, 0, m_previewBtn, m_colorBtn), 2, 0);
    m_root->addWidget(m_draw, 2, 1);

    connect(m_x, &QSpinBox::valueChanged, this, &RectPanel::updatePreview);
    connect(m_y, &QSpinBox::valueChanged, this, &RectPanel::updatePreview);
    connect(m_width, &QSpinBox::valueChanged, this, &RectPanel::updatePreview);
    connect(m_height, &QSpinBox::valueChanged, this, &RectPanel::updatePreview);
    connect(m_colorBtn, &ColorButton::colorChanged, this, &RectPanel::updatePreview);
}

void RectPanel::updateRange(const epd_gfx_canvas_t canvas)
{
    setPointRange(m_x, m_y, canvas);
    m_width->setRange(0, 1000);
    m_width->setValue(100);
    m_height->setRange(0, 1000);
    m_height->setValue(100);
}

DrawFunc RectPanel::drawFunc() const
{
    const int x      = m_x->value();
    const int y      = m_y->value();
    const int width  = m_width->value();
    const int height = m_height->value();
    const auto color = m_colorBtn->currentColor();
    if (m_isDraw) {
        return [x, y, width, height, color](epd_gfx_canvas_t c) {
            return epd_gfx_canvas_draw_rect(c, epd_gfx_rect_t{
                static_cast<uint16_t>(x),
                static_cast<uint16_t>(y),
                static_cast<uint16_t>(width),
                static_cast<uint16_t>(height),
            }, color);
        };
    }
    return [x, y, width, height, color](epd_gfx_canvas_t c) {
        return epd_gfx_canvas_fill_rect(c, epd_gfx_rect_t{
            static_cast<uint16_t>(x),
            static_cast<uint16_t>(y),
            static_cast<uint16_t>(width),
            static_cast<uint16_t>(height),
        }, color);
    };
}

LEKCO_END_NAMESPACE
