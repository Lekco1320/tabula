/**
 * @file MainWindow.h
 * @brief Main window for the tabula desktop client.
 * 
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2025-12-9
 * @license MIT
 */

#pragma once

#ifndef _MAINWINDOW_H_
#define _MAINWINDOW_H_

#include <QMainWindow>
#include <QComboBox>
#include <QPushButton>

#include "common.h"
#include "CanvasPreviewer.hpp"

_LEKCO_BEGIN_NAMESPACE

class MainWindow
    : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    void drawDemo();

    CanvasPreviewer* m_previewer = nullptr;
    QComboBox*       m_rotationCombo = nullptr;
    QPushButton*     m_drawButton = nullptr;
};

_LEKCO_END_NAMESPACE

#endif // !_MAINWINDOW_H_
