/**
 * @file AddGlyphDialog.cpp
 * @brief Dialog implementation for adding glyphs to an EGF font asset.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2026-05-13
 * @license MIT
 */

#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QStackedWidget>
#include <QVBoxLayout>

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

    m_fontPathEdit = new QLineEdit(this);
    auto* browseButton = new QPushButton(QStringLiteral("Browse..."), this);
    connect(browseButton, &QPushButton::clicked, this, &AddGlyphDialog::browseFont);

    auto* fontRow = new QWidget(this);
    auto* fontLayout = new QHBoxLayout(fontRow);
    fontLayout->setContentsMargins(0, 0, 0, 0);
    fontLayout->addWidget(m_fontPathEdit, 1);
    fontLayout->addWidget(browseButton);

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

    auto* form = new QFormLayout;
    form->addRow(QStringLiteral("Font File:"), fontRow);
    form->addRow(QStringLiteral("Size:"), m_sizeSpin);
    form->addRow(QStringLiteral("Mode:"), modeRow);
    form->addRow(QStringLiteral("Codepoint:"), m_codepointStack);

    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &AddGlyphDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(buttonBox);
    setLayout(layout);

    updateMode();
}

QString AddGlyphDialog::fontPath() const
{
    return m_fontPathEdit->text().trimmed();
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

void AddGlyphDialog::accept()
{
    const QFileInfo fontInfo(fontPath());
    if (!fontInfo.isFile()) {
        QMessageBox::critical(this, QStringLiteral("Glyph Error"), QStringLiteral("Font file does not exist."));
        return;
    }

    const QString suffix = fontInfo.suffix();
    if (suffix.compare(QStringLiteral("ttf"), Qt::CaseInsensitive) != 0
        && suffix.compare(QStringLiteral("otf"), Qt::CaseInsensitive) != 0) {
        QMessageBox::critical(this, QStringLiteral("Glyph Error"),
            QStringLiteral("Font file must use the .ttf or .otf extension."));
        return;
    }

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

void AddGlyphDialog::browseFont()
{
    const QString file = QFileDialog::getOpenFileName(this, QStringLiteral("Select Font File"),
        QString(), QStringLiteral("SFNT Fonts (*.ttf *.otf)"));
    if (!file.isEmpty()) {
        m_fontPathEdit->setText(file);
    }
}

void AddGlyphDialog::updateMode()
{
    const bool single = m_singleRadio->isChecked();
    m_codepointStack->setCurrentIndex(single ? 0 : 1);
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
