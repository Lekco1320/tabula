/**
 * @file LinePanel.cpp
 * @brief Panel to configure and draw a line.
 * 
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2025-12-16
 * @license MIT
 */

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QCheckBox>

#include "controls/Utils.hpp"
#include "controls/panels/LinePanel.hpp"
#include "controls/widgets/ColorButton.hpp"
#include "controls/widgets/CanvasPreviewer.hpp"

LEKCO_BEGIN_NAMESPACE

LinePanel::LinePanel(const QString& title, Qt::Orientation orientation, QWidget* parent)
    : ControlPanel(title, parent)
    , m_orientation(orientation)
    , m_x(new QSpinBox(this))
    , m_y(new QSpinBox(this))
    , m_len(new QSpinBox(this))
    , m_colorBtn(new ColorButton(this))
    , m_draw(new QPushButton(QStringLiteral("Draw"), this))
{
    auto* checkBtn = new QCheckBox(QStringLiteral("Preview"), this);
    checkBtn->setStyleSheet("QCheckBox { spacing: 4px; }");
    connect(checkBtn, &QCheckBox::checkStateChanged, [this](int checked) {
        m_enablePreview = (bool)checked;
        updatePreview();
    });

    m_root->addWidget(MakeLabeledWidget(this, QStringLiteral("X:"), m_x), 0, 0);
    m_root->addWidget(MakeLabeledWidget(this, QStringLiteral("Y:"), m_y), 0, 1);
    m_root->addWidget(MakeLabeledWidget(this, QStringLiteral("L:"), m_len), 1, 0);
    m_root->addWidget(MakeRow(this, 0, checkBtn, m_colorBtn), 1, 1);
    m_root->addWidget(m_draw, 2, 1);

    connect(m_x, &QSpinBox::valueChanged, this, &LinePanel::updatePreview);
    connect(m_y, &QSpinBox::valueChanged, this, &LinePanel::updatePreview);
    connect(m_len, &QSpinBox::valueChanged, this, &LinePanel::updatePreview);
    connect(m_draw, &QPushButton::clicked, this, &LinePanel::updateDraw);
    connect(m_colorBtn, &ColorButton::colorChanged, this, &LinePanel::updatePreview);
}

void LinePanel::updateRange(const epd_gfx_canvas_t canvas)
{
    uint16_t width  = epd_gfx_canvas_get_logical_width(canvas);
    uint16_t height = epd_gfx_canvas_get_logical_height(canvas);

    m_x->setRange(1, width);
    m_y->setRange(1, height);
    m_len->setRange(0, 5000);
    m_len->setValue(100);
}

void LinePanel::updateDraw() const
{
    auto func = drawFunc();
    emit drawRequested(func);
}

void LinePanel::updatePreview() const
{
    auto func = drawFunc();
    if (m_enablePreview) {
        emit previewRequested(func);
    } else {
        emit refreshRequested();
    }
}

DrawFunc LinePanel::drawFunc() const
{
    const int x      = m_x->value();
    const int y      = m_y->value();
    const int len    = m_len->value();
    const auto color = m_colorBtn->currentColor();
    if (m_orientation == Qt::Horizontal) {
        return [x, y, len, color](epd_gfx_canvas_t c) {
            epd_gfx_canvas_draw_hline(c, x, y, len, color);
        };
    }
    return [x, y, len, color](epd_gfx_canvas_t c) {
        epd_gfx_canvas_draw_vline(c, x, y, len, color);
    };
}

LEKCO_END_NAMESPACE
