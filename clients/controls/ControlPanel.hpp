/**
 * @file ControlPanel.hpp
 * @brief Base panel for canvas-bound controls.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2025-12-13
 * @license MIT
 */

#pragma once

#ifndef _CONTROLPANEL_HPP_
#define _CONTROLPANEL_HPP_

#include <QGroupBox>
#include <QSize>
#include <QColor>

#include "common.h"
#include "epd_gfx/canvas.h"

class QGridLayout;
class QCheckBox;
class QWidget;

LEKCO_BEGIN_NAMESPACE

class CanvasPreviewer;

class ControlPanel
    : public QGroupBox
{
    Q_OBJECT

public:
    explicit ControlPanel(CanvasPreviewer* previewer, QWidget* parent = nullptr);

protected:
    virtual void flushToCanvas(epd_gfx_canvas_t canvas) = 0;
    virtual void refreshPreview();
    QWidget* makeRow(const QString& label, QWidget* editor) const;

protected:
    CanvasPreviewer* m_previewer;
    QGridLayout*     m_root;
    bool             m_enablePreview;
};

LEKCO_END_NAMESPACE

#endif // !_CONTROLPANEL_HPP_
