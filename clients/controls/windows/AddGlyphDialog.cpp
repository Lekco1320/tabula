/**
 * @file AddGlyphDialog.cpp
 * @brief Dialog implementation for adding glyphs to an EGF font asset.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2026-05-13
 * @license MIT
 */

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QRadioButton>
#include <QSpinBox>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QWidget>

#include "controls/windows/AddGlyphDialog.hpp"

LEKCO_BEGIN_NAMESPACE

BEGIN_NAMESPACE()

bool IsValidUnicodeCodepoint(uint32_t codepoint)
{
    return codepoint <= 0x10FFFFU && (codepoint < 0xD800U || codepoint > 0xDFFFU);
}

END_NAMESPACE

AddGlyphDialog::AddGlyphDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("Add Glyph"));
    setModal(true);

    m_sizeSpin = new QSpinBox(this);
    m_sizeSpin->setRange(1, 65535);
    m_sizeSpin->setValue(16);

    m_singleRadio = new QRadioButton(QStringLiteral("Single Codepoint"), this);
    m_rangeRadio  = new QRadioButton(QStringLiteral("Codepoint Range"), this);
    m_singleRadio->setChecked(true);
    connect(m_singleRadio, &QRadioButton::toggled, this, &AddGlyphDialog::updateMode);

    m_singleCodepoint = new QLineEdit(this);

    m_rangeStartCodepoint = new QLineEdit(this);
    m_rangeEndCodepoint = new QLineEdit(this);

    auto* modeRow = new QWidget(this);
    auto* modeLayout = new QHBoxLayout(modeRow);
    modeLayout->setContentsMargins(0, 0, 0, 0);
    modeLayout->addWidget(m_singleRadio);
    modeLayout->addWidget(m_rangeRadio);
    modeLayout->addStretch(1);

    auto* singleRow    = new QWidget(this);
    auto* singleLayout = new QHBoxLayout(singleRow);
    singleLayout->setContentsMargins(0, 0, 0, 0);
    singleLayout->addWidget(new QLabel(QStringLiteral("U+"), singleRow));
    singleLayout->addWidget(m_singleCodepoint, 1);

    auto* rangeRow     = new QWidget(this);
    auto* rangeLayout  = new QHBoxLayout(rangeRow);
    rangeLayout->setContentsMargins(0, 0, 0, 0);
    rangeLayout->addWidget(new QLabel(QStringLiteral("U+"), rangeRow));
    rangeLayout->addWidget(m_rangeStartCodepoint, 1);
    rangeLayout->addWidget(new QLabel(QStringLiteral("-"), rangeRow));
    rangeLayout->addWidget(new QLabel(QStringLiteral("U+"), rangeRow));
    rangeLayout->addWidget(m_rangeEndCodepoint, 1);

    m_codepointStack = new QStackedWidget(this);
    m_codepointStack->addWidget(singleRow);
    m_codepointStack->addWidget(rangeRow);

    m_renderModeCombo = new QComboBox(this);
    m_renderModeCombo->addItem(QStringLiteral("Monochrome"),
        EPD_ASSET_FONT_FACE_RENDER_MONO);
    m_renderModeCombo->addItem(QStringLiteral("Grayscale Threshold"),
        EPD_ASSET_FONT_FACE_RENDER_GRAY_THRESHOLD);
    connect(m_renderModeCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
        this, &AddGlyphDialog::updateRenderMode);

    auto* grayParams      = new QWidget(this);
    auto* grayParamLayout = new QFormLayout(grayParams);
    grayParamLayout->setContentsMargins(0, 0, 0, 0);

    m_thresholdSpin = new QSpinBox(grayParams);
    m_thresholdSpin->setRange(0, 255);
    m_thresholdSpin->setValue(128);

    m_biasSpin = new QSpinBox(grayParams);
    m_biasSpin->setRange(-128, 127);
    m_biasSpin->setValue(0);

    grayParamLayout->addRow(QStringLiteral("Threshold:"), m_thresholdSpin);
    grayParamLayout->addRow(QStringLiteral("Bias:"), m_biasSpin);

    m_renderParamStack = new QStackedWidget(this);
    m_renderParamStack->addWidget(grayParams);

    auto* form = new QFormLayout;
    form->addRow(QStringLiteral("Size:"), m_sizeSpin);
    form->addRow(QStringLiteral("Mode:"), modeRow);
    form->addRow(QStringLiteral("Codepoint:"), m_codepointStack);
    form->addRow(QStringLiteral("Render Mode:"), m_renderModeCombo);

    m_renderParamRow = new QWidget(this);
    auto* renderParamLayout = new QFormLayout(m_renderParamRow);
    renderParamLayout->setContentsMargins(0, 0, 0, 0);
    renderParamLayout->addRow(QStringLiteral("Render Parameters:"), m_renderParamStack);

    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &AddGlyphDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(m_renderParamRow);
    layout->addWidget(buttonBox);
    setLayout(layout);

    updateMode();
    updateRenderMode();
}

uint16_t AddGlyphDialog::size() const
{
    return static_cast<uint16_t>(m_sizeSpin->value());
}

uint32_t AddGlyphDialog::startCodepoint() const
{
    return m_startCodepoint;
}

uint32_t AddGlyphDialog::endCodepoint() const
{
    return m_endCodepoint;
}

epd_asset_font_face_render_mode_t AddGlyphDialog::renderMode() const
{
    return static_cast<epd_asset_font_face_render_mode_t>(m_renderModeCombo->currentData().toInt());
}

uint8_t AddGlyphDialog::threshold() const
{
    return static_cast<uint8_t>(m_thresholdSpin->value());
}

int8_t AddGlyphDialog::bias() const
{
    return static_cast<int8_t>(m_biasSpin->value());
}

void AddGlyphDialog::accept()
{
    uint32_t start = 0U;
    uint32_t end   = 0U;
    if (m_singleRadio->isChecked()) {
        if (!parseCodepoint(m_singleCodepoint->text(), &start)) {
            QMessageBox::critical(this, QStringLiteral("Glyph Error"), QStringLiteral("Invalid codepoint."));
            return;
        }
        end = start;
    } else {
        if (!parseCodepoint(m_rangeStartCodepoint->text(), &start)
            || !parseCodepoint(m_rangeEndCodepoint->text(), &end) || start > end) {
            QMessageBox::critical(this, QStringLiteral("Glyph Error"), QStringLiteral("Invalid codepoint range."));
            return;
        }
    }

    m_startCodepoint = start;
    m_endCodepoint   = end;
    QDialog::accept();
}

void AddGlyphDialog::updateMode()
{
    const bool single = m_singleRadio->isChecked();
    m_codepointStack->setCurrentIndex(single ? 0 : 1);
}

void AddGlyphDialog::updateRenderMode(int index)
{
    Q_UNUSED(index)

    const bool showParams = renderMode() == EPD_ASSET_FONT_FACE_RENDER_GRAY_THRESHOLD;
    m_renderParamRow->setVisible(showParams);
    if (showParams) {
        m_renderParamStack->setCurrentIndex(0);
    }
    adjustSize();
}

bool AddGlyphDialog::parseCodepoint(const QString& text, uint32_t* out_codepoint)
{
    QString value = text.trimmed();
    if (value.startsWith(QStringLiteral("U+"), Qt::CaseInsensitive)) {
        value = value.mid(2);
    } else if (value.startsWith(QStringLiteral("0x"), Qt::CaseInsensitive)) {
        value = value.mid(2);
    }
    if (value.isEmpty()) {
        return false;
    }

    bool ok = false;
    const uint codepoint = value.toUInt(&ok, 16);
    if (!ok || !IsValidUnicodeCodepoint(codepoint)) {
        return false;
    }

    *out_codepoint = codepoint;
    return true;
}

LEKCO_END_NAMESPACE
