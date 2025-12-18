/**
 * @file MainWindow.hpp
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

#include "common/common.h"
#include "controls/widgets/CanvasPreviewer.hpp"

LEKCO_BEGIN_NAMESPACE

class ToolBar;
class CursorBar;
class AdaptiveStackedWidget;

class MainWindow
    : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private:
    void drawDemo();

    CursorBar*             m_cursorBar     = nullptr;
    ToolBar*               m_toolBar       = nullptr;
    CanvasPreviewer*       m_previewer     = nullptr;
    AdaptiveStackedWidget* m_stackedWidget = nullptr;
    QComboBox*             m_rotationCombo = nullptr;
    QPushButton*           m_drawButton    = nullptr;
};

LEKCO_END_NAMESPACE

#endif // !_MAINWINDOW_H_
