/**
 * @file ControlPanel.cpp
 * @brief Base panel for canvas-bound controls.
 * 
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2025-12-13
 * @license MIT
 */

#include <QGridLayout>
#include <QCheckBox>
#include <QLabel>

#include "controls/panels/ControlPanel.hpp"
#include "controls/widgets/CanvasPreviewer.hpp"

LEKCO_BEGIN_NAMESPACE

ControlPanel::ControlPanel(const QString& title, QWidget* parent)
    : QGroupBox(title, parent)
    , m_root(new QGridLayout(this))
    , m_enablePreview(false)
{
    m_root->setContentsMargins(8, 8, 8, 8);
    m_root->setHorizontalSpacing(10);
    m_root->setVerticalSpacing(5);
}

LEKCO_END_NAMESPACE
