/**
 * @file FillPanel.cpp
 * @brief Panel to configure and fill previewer in an specified color.
 * 
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2025-12-21
 * @license MIT
 */

#include <QCheckBox>
#include <QPushButton>

#include "controls/panels/FillPanel.hpp"
#include "controls/widgets/ColorButton.hpp"

LEKCO_BEGIN_NAMESPACE

FillPanel::FillPanel(const QString& title, QWidget* parent)
    : ControlPanel(title, parent)
    , m_previewBtn(new QCheckBox(QStringLiteral("Preview"), this))
    , m_colorBtn(new ColorButton(this))
    , m_draw(new QPushButton(QStringLiteral("Draw"), this))
{
    m_previewBtn->setStyleSheet(QStringLiteral("QCheckBox { spacing: 4px; }"));
    connect(m_previewBtn, &QCheckBox::checkStateChanged, [this](int checked) {
        m_enablePreview = (bool)checked;
        updatePreview();
    });

    m_root->addWidget(MakeRow(this, 0, m_previewBtn, m_colorBtn), 1, 0);
    m_root->addWidget(m_draw, 1, 1);

    connect(m_draw, &QPushButton::clicked, this, &FillPanel::updateDraw);
    connect(m_colorBtn, &ColorButton::colorChanged, this, &FillPanel::updatePreview);
}

DrawFunc FillPanel::drawFunc() const
{
    const auto color = m_colorBtn->currentColor();
    return [color](epd_gfx_canvas_t c) {
        return epd_gfx_canvas_fill(c, color);
    };
}

LEKCO_END_NAMESPACE
