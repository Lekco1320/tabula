/**
 * @file LinePanel.cpp
 * @brief Panel to configure and draw a line.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2025-12-16
 * @license MIT
 */

#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>

#include "controls/Utils.hpp"
#include "controls/panels/LinePanel.hpp"
#include "controls/widgets/ColorButton.hpp"

LEKCO_BEGIN_NAMESPACE

LinePanel::LinePanel(const QString& title, Qt::Orientation orientation, QWidget* parent)
    : ControlPanel(title, parent)
    , m_orientation(orientation)
    , m_x(new QSpinBox(this))
    , m_y(new QSpinBox(this))
    , m_len(new QSpinBox(this))
    , m_previewBtn(createPreviewCheckBox())
    , m_colorBtn(new ColorButton(this))
    , m_draw(createDrawButton())
{
    m_root->addWidget(MakeLabeledWidget(this, QStringLiteral("X:"), m_x), 0, 0);
    m_root->addWidget(MakeLabeledWidget(this, QStringLiteral("Y:"), m_y), 0, 1);
    m_root->addWidget(MakeLabeledWidget(this, QStringLiteral("L:"), m_len), 1, 0);
    m_root->addWidget(MakeRow(this, 0, m_previewBtn, m_colorBtn), 1, 1);
    m_root->addWidget(m_draw, 2, 1);

    connect(m_x, &QSpinBox::valueChanged, this, &LinePanel::updatePreview);
    connect(m_y, &QSpinBox::valueChanged, this, &LinePanel::updatePreview);
    connect(m_len, &QSpinBox::valueChanged, this, &LinePanel::updatePreview);
    connect(m_colorBtn, &ColorButton::colorChanged, this, &LinePanel::updatePreview);
}

void LinePanel::updateRange(const epd_gfx_canvas_t canvas)
{
    setPointRange(m_x, m_y, canvas);
    m_len->setRange(0, 1000);
    m_len->setValue(100);
}

DrawFunc LinePanel::drawFunc() const
{
    const int x      = m_x->value();
    const int y      = m_y->value();
    const int len    = m_len->value();
    const auto color = m_colorBtn->currentColor();
    if (m_orientation == Qt::Horizontal) {
        return [x, y, len, color](epd_gfx_canvas_t c) {
            return epd_gfx_canvas_draw_hline(c, epd_gfx_point_t{
                static_cast<uint16_t>(x),
                static_cast<uint16_t>(y),
            }, static_cast<uint16_t>(len), color);
        };
    }
    return [x, y, len, color](epd_gfx_canvas_t c) {
        return epd_gfx_canvas_draw_vline(c, epd_gfx_point_t{
            static_cast<uint16_t>(x),
            static_cast<uint16_t>(y),
        }, static_cast<uint16_t>(len), color);
    };
}

LEKCO_END_NAMESPACE
