/**
 * @file NewFontDialog.cpp
 * @brief New EGF font dialog implementation for project assets.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2026-05-13
 * @license MIT
 */

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QVBoxLayout>

#include "controls/windows/NewFontDialog.hpp"
#include "project/Project.hpp"

LEKCO_BEGIN_NAMESPACE

NewFontDialog::NewFontDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("New Font"));
    setModal(true);

    m_fontNameEdit = new QLineEdit(this);
    m_fontNameEdit->setMaxLength(64);

    auto* form = new QFormLayout;
    form->addRow(QStringLiteral("Font Name:"), m_fontNameEdit);

    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &NewFontDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(buttonBox);
    setLayout(layout);
}

QString NewFontDialog::fontName() const
{
    return m_fontNameEdit->text();
}

void NewFontDialog::accept()
{
    QString error;
    if (!Project::validateFontName(fontName(), &error)) {
        QMessageBox::critical(this, QStringLiteral("Font Error"), error);
        return;
    }

    QDialog::accept();
}

LEKCO_END_NAMESPACE
