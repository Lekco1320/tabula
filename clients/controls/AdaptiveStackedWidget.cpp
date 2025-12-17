/**
 * @file AdaptiveStackedWidget.cpp
 * @brief Stacked widget whose size follows the current page.
 * 
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2025-12-17
 * @license MIT
 */

#include "AdaptiveStackedWidget.hpp"

LEKCO_BEGIN_NAMESPACE

AdaptiveStackedWidget::AdaptiveStackedWidget(QWidget* parent)
    : QStackedWidget(parent)
{
    connect(this, &QStackedWidget::currentChanged, this, &QWidget::updateGeometry);
}

void AdaptiveStackedWidget::setCollapsed(bool collapsed)
{
    if (m_collapsed == collapsed) {
        return;
    }
    m_collapsed = collapsed;
    updateGeometry();
}

bool AdaptiveStackedWidget::isCollapsed() const
{
    return m_collapsed;
}

QSize AdaptiveStackedWidget::sizeHint() const
{
    if (m_collapsed) {
        return QSize(0, 0);
    }
    if (auto* w = currentWidget()) {
        return w->sizeHint();
    }
    return QStackedWidget::sizeHint();
}

QSize AdaptiveStackedWidget::minimumSizeHint() const
{
    if (m_collapsed) {
        return QSize(0, 0);
    }
    if (auto* w = currentWidget()) {
        return w->minimumSizeHint();
    }
    return QStackedWidget::minimumSizeHint();
}

LEKCO_END_NAMESPACE
