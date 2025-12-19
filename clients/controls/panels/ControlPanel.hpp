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

#include <functional>
#include <QString>
#include <QGroupBox>
#include <QSize>
#include <QColor>
#include <epd_gfx/canvas.h>

#include "common/common.h"
#include "controls/Utils.hpp"

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
    explicit ControlPanel(const QString& title, QWidget* parent = nullptr);
    virtual void updateDraw() const = 0;
    virtual void updatePreview() const = 0;
    virtual void updateRange(epd_gfx_canvas_t canvas) = 0;

signals:
    void refreshRequested() const;
    void drawRequested(const DrawFunc& drawFunc) const;
    void previewRequested(const DrawFunc& drawFunc) const;

protected:
    virtual DrawFunc drawFunc() const = 0;

    QGridLayout* m_root;
    bool         m_enablePreview;
};

LEKCO_END_NAMESPACE

#endif // !_CONTROLPANEL_HPP_
