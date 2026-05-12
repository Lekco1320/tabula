/**
 * @file SetupDialog.cpp
 * @brief Implementation of the setup dialog.
 * 
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2025-12-21
 * @license MIT
 */

#include <QSpinBox>
#include <QComboBox>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QDialogButtonBox>

#include "controls/windows/SetupDialog.hpp"
#include "common/Common.h"

LEKCO_BEGIN_NAMESPACE

SetupDialog::SetupDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("Screen Setup"));
    setModal(true);

    m_widthSpinBox = new QSpinBox(this);
    m_widthSpinBox->setRange(1, 4096);
    m_widthSpinBox->setValue(640);

    m_heightSpinBox = new QSpinBox(this);
    m_heightSpinBox->setRange(1, 4096);
    m_heightSpinBox->setValue(384);

    m_formatComboBox = new QComboBox(this);
    m_formatComboBox->addItem(QStringLiteral("Native"), QVariant::fromValue(EPD_GFX_FORMAT_NATIVE));
    m_formatComboBox->addItem(QStringLiteral("Planes"), QVariant::fromValue(EPD_GFX_FORMAT_PLANES));

    m_rotationComboBox = new QComboBox(this);
    m_rotationComboBox->addItem(QStringLiteral("0°"), QVariant::fromValue(EPD_GFX_ROTATE_0));
    m_rotationComboBox->addItem(QStringLiteral("90°"), QVariant::fromValue(EPD_GFX_ROTATE_90));
    m_rotationComboBox->addItem(QStringLiteral("180°"), QVariant::fromValue(EPD_GFX_ROTATE_180));
    m_rotationComboBox->addItem(QStringLiteral("270°"), QVariant::fromValue(EPD_GFX_ROTATE_270));

    auto* formLayout = new QFormLayout;
    formLayout->addRow(QStringLiteral("Width:"), m_widthSpinBox);
    formLayout->addRow(QStringLiteral("Height:"), m_heightSpinBox);
    formLayout->addRow(QStringLiteral("Format:"), m_formatComboBox);
    formLayout->addRow(QStringLiteral("Rotation:"), m_rotationComboBox);

    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(formLayout);
    mainLayout->addSpacing(10);
    mainLayout->addWidget(buttonBox);

    setLayout(mainLayout);
}

int SetupDialog::panelWidth() const
{
    return m_widthSpinBox->value();
}

int SetupDialog::panelHeight() const
{
    return m_heightSpinBox->value();
}

epd_gfx_format_t SetupDialog::format() const
{
    return m_formatComboBox->currentData().value<epd_gfx_format_t>();
}

epd_gfx_rotation_t SetupDialog::rotation() const
{
    return m_rotationComboBox->currentData().value<epd_gfx_rotation_t>();
}

LEKCO_END_NAMESPACE
