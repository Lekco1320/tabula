/**
 * @file NewBitmapDialog.cpp
 * @brief New EBM bitmap dialog implementation for project assets.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2026-05-20
 * @license MIT
 */

#include <stdint.h>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QImageReader>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QWidget>

#include "controls/windows/NewBitmapDialog.hpp"
#include "project/Project.hpp"

LEKCO_BEGIN_NAMESPACE

NewBitmapDialog::NewBitmapDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("New Bitmap"));
    setModal(true);

    m_bitmapNameEdit = new QLineEdit(this);
    m_bitmapNameEdit->setMaxLength(64);

    m_sourceImageEdit = new QLineEdit(this);
    connect(m_sourceImageEdit, &QLineEdit::textChanged, this, &NewBitmapDialog::refreshSize);
    auto* browseButton = new QPushButton(QStringLiteral("Browse..."), this);
    connect(browseButton, &QPushButton::clicked, this, &NewBitmapDialog::browseSourceImage);

    auto* sourceRow = new QWidget(this);
    auto* sourceLayout = new QHBoxLayout(sourceRow);
    sourceLayout->setContentsMargins(0, 0, 0, 0);
    sourceLayout->addWidget(m_sourceImageEdit, 1);
    sourceLayout->addWidget(browseButton);

    m_widthSpin = new QSpinBox(this);
    m_widthSpin->setRange(1, UINT16_MAX);
    m_widthSpin->setValue(1);

    m_heightSpin = new QSpinBox(this);
    m_heightSpin->setRange(1, UINT16_MAX);
    m_heightSpin->setValue(1);

    auto* sizeRow = new QWidget(this);
    auto* sizeLayout = new QHBoxLayout(sizeRow);
    sizeLayout->setContentsMargins(0, 0, 0, 0);
    sizeLayout->setSpacing(4);
    sizeLayout->addWidget(m_widthSpin);
    sizeLayout->addWidget(m_heightSpin);

    m_formatCombo = new QComboBox(this);
    m_formatCombo->addItem(QStringLiteral("Native"), EPD_GFX_FORMAT_NATIVE);
    m_formatCombo->addItem(QStringLiteral("Planes"), EPD_GFX_FORMAT_PLANES);

    auto* form = new QFormLayout;
    form->addRow(QStringLiteral("Bitmap Name:"), m_bitmapNameEdit);
    form->addRow(QStringLiteral("Source Image:"), sourceRow);
    form->addRow(QStringLiteral("W / H:"), sizeRow);
    form->addRow(QStringLiteral("Output Format:"), m_formatCombo);

    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &NewBitmapDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(buttonBox);
    setLayout(layout);
}

QString NewBitmapDialog::bitmapName() const
{
    return m_bitmapNameEdit->text();
}

QString NewBitmapDialog::sourceImagePath() const
{
    return m_sourceImageEdit->text().trimmed();
}

uint16_t NewBitmapDialog::targetWidth() const
{
    return static_cast<uint16_t>(m_widthSpin->value());
}

uint16_t NewBitmapDialog::targetHeight() const
{
    return static_cast<uint16_t>(m_heightSpin->value());
}

epd_gfx_format_t NewBitmapDialog::outputFormat() const
{
    return static_cast<epd_gfx_format_t>(m_formatCombo->currentData().toInt());
}

void NewBitmapDialog::accept()
{
    QString error;
    if (!Project::validateBitmapName(bitmapName(), &error)) {
        QMessageBox::critical(this, QStringLiteral("Bitmap Error"), error);
        return;
    }

    const QFileInfo sourceInfo(sourceImagePath());
    if (!sourceInfo.isFile()) {
        QMessageBox::critical(this, QStringLiteral("Bitmap Error"), QStringLiteral("Source image file does not exist."));
        return;
    }

    QImageReader reader(sourceInfo.absoluteFilePath());
    const QSize imageSize = reader.size();
    if (!reader.canRead() || !imageSize.isValid()) {
        QMessageBox::critical(this, QStringLiteral("Bitmap Error"), QStringLiteral("Source image file is not supported."));
        return;
    }

    QDialog::accept();
}

void NewBitmapDialog::browseSourceImage()
{
    const QString file = QFileDialog::getOpenFileName(this, QStringLiteral("Select Source Image"),
        QString(), QStringLiteral("Images (*.png *.jpg *.jpeg *.bmp *.webp)"));
    if (file.isEmpty()) {
        return;
    }

    m_sourceImageEdit->setText(file);
    if (m_bitmapNameEdit->text().isEmpty()) {
        m_bitmapNameEdit->setText(QFileInfo(file).completeBaseName());
    }

}

void NewBitmapDialog::refreshSize()
{
    QImageReader reader(sourceImagePath());
    const QSize imageSize = reader.size();
    if (!imageSize.isValid()) {
        return;
    }

    m_widthSpin->setValue(imageSize.width() > UINT16_MAX ? UINT16_MAX : imageSize.width());
    m_heightSpin->setValue(imageSize.height() > UINT16_MAX ? UINT16_MAX : imageSize.height());
}

LEKCO_END_NAMESPACE
