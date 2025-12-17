/**
 * @file AdaptiveStackedWidget.hpp
 * @brief Stacked widget whose size follows the current page.
 * 
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2025-12-17
 * @license MIT
 */

#pragma once

#ifndef _ADAPTIVESTACKEDWIDGET_HPP_
#define _ADAPTIVESTACKEDWIDGET_HPP_

#include <QStackedWidget>

#include "common.h"

LEKCO_BEGIN_NAMESPACE

class AdaptiveStackedWidget
    : public QStackedWidget
{
    Q_OBJECT

public:
    explicit AdaptiveStackedWidget(QWidget* parent = nullptr);

    void  setCollapsed(bool collapsed);
    bool  isCollapsed() const;
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

private:
    bool m_collapsed = false;
};

LEKCO_END_NAMESPACE

#endif // !_ADAPTIVESTACKEDWIDGET_HPP_
