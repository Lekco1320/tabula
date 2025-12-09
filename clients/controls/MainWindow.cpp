/**
 * @file MainWindow.cpp
 * @brief Main window implementation for the tabula desktop client.
 * 
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2025-12-9
 * @license MIT
 */

#include <QComboBox>
#include <QVBoxLayout>
#include <QWidget>

#include "MainWindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setMinimumSize(QSize { 640, 384 });
    setMaximumSize(QSize { 640, 384 });

    auto* central = new QWidget(this);
    auto* layout  = new QVBoxLayout(central);

    QComboBox* comboBox = new QComboBox(central);
    comboBox->addItem(QStringLiteral("ABC"));
    comboBox->addItem(QStringLiteral("BDE"));
    comboBox->addItem(QStringLiteral("CDF"));

    layout->addStretch();
    layout->addWidget(comboBox, 0, Qt::AlignCenter);
    layout->addStretch();

    setCentralWidget(central);
}

MainWindow::~MainWindow()
{
}
