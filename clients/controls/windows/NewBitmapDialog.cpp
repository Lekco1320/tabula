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
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSlider>
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

    m_scaleSlider = new QSlider(Qt::Horizontal, this);
    m_scaleSlider->setRange(1, 400);
    m_scaleSlider->setValue(100);
    connect(m_scaleSlider, &QSlider::valueChanged, this, &NewBitmapDialog::refreshSize);

    m_scaleLabel = new QLabel(QStringLiteral("100%"), this);
    m_scaleLabel->setFixedWidth(fontMetrics().horizontalAdvance(QStringLiteral("400%")) + 4);
    m_scaleLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    m_sizeLabel = new QLabel(QStringLiteral("-"), this);
    m_sizeLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    auto* scaleRow = new QWidget(this);
    auto* scaleLayout = new QHBoxLayout(scaleRow);
    scaleLayout->setContentsMargins(0, 0, 0, 0);
    scaleLayout->setSpacing(4);
    scaleLayout->addWidget(m_scaleSlider, 1);
    scaleLayout->addWidget(m_scaleLabel);
    scaleLayout->addWidget(m_sizeLabel);

    m_formatCombo = new QComboBox(this);
    m_formatCombo->addItem(QStringLiteral("Native"), EPD_GFX_FORMAT_NATIVE);
    m_formatCombo->addItem(QStringLiteral("Planes"), EPD_GFX_FORMAT_PLANES);

    auto* form = new QFormLayout;
    form->addRow(QStringLiteral("Bitmap Name:"), m_bitmapNameEdit);
    form->addRow(QStringLiteral("Source Image:"), sourceRow);
    form->addRow(QStringLiteral("Scale:"), scaleRow);
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
    const int width = (m_sourceWidth * m_scaleSlider->value() + 50) / 100;
    return width > 0 && width <= UINT16_MAX ? static_cast<uint16_t>(width) : 0U;
}

uint16_t NewBitmapDialog::targetHeight() const
{
    const int height = (m_sourceHeight * m_scaleSlider->value() + 50) / 100;
    return height > 0 && height <= UINT16_MAX ? static_cast<uint16_t>(height) : 0U;
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

    m_sourceWidth  = imageSize.width();
    m_sourceHeight = imageSize.height();
    if (targetWidth() == 0U || targetHeight() == 0U) {
        QMessageBox::critical(this, QStringLiteral("Bitmap Error"),
            QStringLiteral("Bitmap size must be in range 1..65535."));
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

    refreshSize();
}

void NewBitmapDialog::refreshSize()
{
    QImageReader reader(sourceImagePath());
    const QSize imageSize = reader.size();
    if (imageSize.isValid()) {
        m_sourceWidth  = imageSize.width();
        m_sourceHeight = imageSize.height();
    } else {
        m_sourceWidth  = 0;
        m_sourceHeight = 0;
    }

    if (m_sourceWidth <= 0 || m_sourceHeight <= 0) {
        m_scaleLabel->setText(QStringLiteral("%1%").arg(m_scaleSlider->value()));
        m_sizeLabel->setText(QStringLiteral("-"));
        return;
    }

    if (targetWidth() == 0U || targetHeight() == 0U) {
        m_scaleLabel->setText(QStringLiteral("%1%").arg(m_scaleSlider->value()));
        m_sizeLabel->setText(QStringLiteral("out of range"));
        return;
    }

    m_scaleLabel->setText(QStringLiteral("%1%").arg(m_scaleSlider->value()));
    m_sizeLabel->setText(QStringLiteral("%1 x %2")
        .arg(targetWidth())
        .arg(targetHeight()));
}

LEKCO_END_NAMESPACE
