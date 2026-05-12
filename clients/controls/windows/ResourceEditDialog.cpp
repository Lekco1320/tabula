/**
 * @file ResourceEditDialog.cpp
 * @brief Resource edit dialog implementation for project assets.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2026-05-12
 * @license MIT
 */

#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

#include "controls/windows/ResourceEditDialog.hpp"

LEKCO_BEGIN_NAMESPACE

ResourceEditDialog::ResourceEditDialog(ProjectResourceType type, const QString& fileName, QWidget* parent)
    : QDialog(parent)
    , m_type(type)
{
    setWindowTitle(QStringLiteral("Edit %1").arg(Project::displayName(type)));
    setModal(true);

    auto* typeLabel = new QLabel(Project::displayName(type), this);

    m_fileNameEdit = new QLineEdit(fileName, this);

    m_replacementEdit = new QLineEdit(this);
    auto* browseButton = new QPushButton(QStringLiteral("Browse..."), this);
    connect(browseButton, &QPushButton::clicked, this, &ResourceEditDialog::browseReplacement);

    auto* replacementRow = new QWidget(this);
    auto* replacementLayout = new QHBoxLayout(replacementRow);
    replacementLayout->setContentsMargins(0, 0, 0, 0);
    replacementLayout->addWidget(m_replacementEdit, 1);
    replacementLayout->addWidget(browseButton);

    auto* form = new QFormLayout;
    form->addRow(QStringLiteral("Type:"), typeLabel);
    form->addRow(QStringLiteral("File Name:"), m_fileNameEdit);
    form->addRow(QStringLiteral("Replace With:"), replacementRow);

    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(buttonBox);
    setLayout(layout);
}

QString ResourceEditDialog::fileName() const
{
    return m_fileNameEdit->text().trimmed();
}

QString ResourceEditDialog::replacementPath() const
{
    return m_replacementEdit->text().trimmed();
}

void ResourceEditDialog::browseReplacement()
{
    const QString file = QFileDialog::getOpenFileName(this, QStringLiteral("Select Replacement File"),
        QString(), Project::fileDialogFilter(m_type));
    if (!file.isEmpty()) {
        m_replacementEdit->setText(file);
    }
}

LEKCO_END_NAMESPACE
