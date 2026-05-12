/**
 * @file ProjectDialog.cpp
 * @brief Project startup dialog implementation for the tabula desktop client.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2026-05-12
 * @license MIT
 */

#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

#include "controls/windows/ProjectDialog.hpp"

LEKCO_BEGIN_NAMESPACE

ProjectDialog::ProjectDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("Tabula"));
    setModal(true);
    setFixedWidth(260);

    auto* newButton    = new QPushButton(QStringLiteral("New Project"), this);
    auto* openButton   = new QPushButton(QStringLiteral("Open Project"), this);
    auto* cancelButton = new QPushButton(QStringLiteral("Cancel"), this);

    connect(newButton, &QPushButton::clicked, this, &ProjectDialog::newProject);
    connect(openButton, &QPushButton::clicked, this, &ProjectDialog::openProject);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(8);
    layout->addWidget(newButton);
    layout->addWidget(openButton);
    layout->addWidget(cancelButton);
    setLayout(layout);
}

Project ProjectDialog::project() const
{
    return m_project;
}

void ProjectDialog::newProject()
{
    const QString dir = QFileDialog::getExistingDirectory(this, QStringLiteral("Select Project Directory"));
    if (dir.isEmpty()) {
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle(QStringLiteral("New Project"));
    dialog.setModal(true);

    auto* widthSpinBox = new QSpinBox(&dialog);
    widthSpinBox->setRange(1, 4096);
    widthSpinBox->setValue(640);

    auto* heightSpinBox = new QSpinBox(&dialog);
    heightSpinBox->setRange(1, 4096);
    heightSpinBox->setValue(384);

    auto* createButton = new QPushButton(QStringLiteral("Create"), &dialog);
    auto* cancelButton = new QPushButton(QStringLiteral("Cancel"), &dialog);
    connect(createButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    auto* buttonRow = new QWidget(&dialog);
    auto* buttonLayout = new QHBoxLayout(buttonRow);
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->addStretch(1);
    buttonLayout->addWidget(createButton);
    buttonLayout->addWidget(cancelButton);

    auto* form = new QFormLayout;
    form->addRow(QStringLiteral("Width:"), widthSpinBox);
    form->addRow(QStringLiteral("Height:"), heightSpinBox);

    auto* layout = new QVBoxLayout(&dialog);
    layout->addLayout(form);
    layout->addWidget(buttonRow);
    dialog.setLayout(layout);

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    ProjectScreen screen;
    screen.width  = widthSpinBox->value();
    screen.height = heightSpinBox->value();

    Project project;
    QString error;
    if (!project.create(dir, screen, &error)) {
        showProjectError(error);
        return;
    }

    m_project = project;
    accept();
}

void ProjectDialog::openProject()
{
    const QString file = QFileDialog::getOpenFileName(this, QStringLiteral("Open Project"),
        QString(), QStringLiteral("Tabula Project (manifest.json);;All Files (*)"));
    if (file.isEmpty()) {
        return;
    }

    Project project;
    QString error;
    if (!project.open(file, &error)) {
        showProjectError(error);
        return;
    }

    m_project = project;
    accept();
}

bool ProjectDialog::showProjectError(const QString& error)
{
    QMessageBox::critical(this, QStringLiteral("Project Error"), error);
    return false;
}

LEKCO_END_NAMESPACE
