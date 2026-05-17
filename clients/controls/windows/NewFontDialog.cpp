/**
 * @file NewFontDialog.cpp
 * @brief New EGF font dialog implementation for project assets.
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
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

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

    m_sourceFontEdit = new QLineEdit(this);
    auto* browseButton = new QPushButton(QStringLiteral("Browse..."), this);
    connect(browseButton, &QPushButton::clicked, this, &NewFontDialog::browseSourceFont);

    auto* sourceRow = new QWidget(this);
    auto* sourceLayout = new QHBoxLayout(sourceRow);
    sourceLayout->setContentsMargins(0, 0, 0, 0);
    sourceLayout->addWidget(m_sourceFontEdit, 1);
    sourceLayout->addWidget(browseButton);

    auto* form = new QFormLayout;
    form->addRow(QStringLiteral("Font Name:"), m_fontNameEdit);
    form->addRow(QStringLiteral("Source Font:"), sourceRow);

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

QString NewFontDialog::sourceFontPath() const
{
    return m_sourceFontEdit->text().trimmed();
}

void NewFontDialog::accept()
{
    QString error;
    if (!Project::validateFontName(fontName(), &error)) {
        QMessageBox::critical(this, QStringLiteral("Font Error"), error);
        return;
    }

    const QFileInfo sourceInfo(sourceFontPath());
    if (!sourceInfo.isFile()) {
        QMessageBox::critical(this, QStringLiteral("Font Error"), QStringLiteral("Source font file does not exist."));
        return;
    }

    const QString suffix = sourceInfo.suffix();
    if (suffix.compare(QStringLiteral("ttf"), Qt::CaseInsensitive) != 0
        && suffix.compare(QStringLiteral("otf"), Qt::CaseInsensitive) != 0) {
        QMessageBox::critical(this, QStringLiteral("Font Error"),
            QStringLiteral("Source font file must use the .ttf or .otf extension."));
        return;
    }

    QDialog::accept();
}

void NewFontDialog::browseSourceFont()
{
    const QString file = QFileDialog::getOpenFileName(this, QStringLiteral("Select Source Font"),
        QString(), QStringLiteral("SFNT Fonts (*.ttf *.otf)"));
    if (!file.isEmpty()) {
        m_sourceFontEdit->setText(file);
    }
}

LEKCO_END_NAMESPACE
