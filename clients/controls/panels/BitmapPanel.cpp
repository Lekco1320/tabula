/**
 * @file BitmapPanel.cpp
 * @brief Panel to configure and draw a bitmap.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2026-05-21
 * @license MIT
 */

#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QtGlobal>
#include <epd_core/common.h>

#include "controls/panels/BitmapPanel.hpp"
#include "project/BitmapProvider.hpp"

LEKCO_BEGIN_NAMESPACE

BitmapPanel::BitmapPanel(const QString& title, BitmapProvider* bitmapProvider, QWidget* parent)
    : ControlPanel(title, parent)
    , m_bitmapProvider(bitmapProvider)
    , m_bitmap(new QComboBox(this))
    , m_size(new QLabel(QStringLiteral("-"), this))
    , m_x(new QSpinBox(this))
    , m_y(new QSpinBox(this))
    , m_previewBtn(createPreviewCheckBox())
    , m_draw(createDrawButton())
{
    m_root->addWidget(MakeLabeledWidget(this, QStringLiteral("Bitmap:"), m_bitmap), 0, 0, 1, 2);
    m_root->addWidget(MakeLabeledWidget(this, QStringLiteral("Size:"), m_size), 1, 0, 1, 2);
    m_root->addWidget(MakeLabeledWidget(this, QStringLiteral("X:"), m_x), 2, 0);
    m_root->addWidget(MakeLabeledWidget(this, QStringLiteral("Y:"), m_y), 2, 1);
    m_root->addWidget(m_previewBtn, 3, 0);
    m_root->addWidget(m_draw, 3, 1, Qt::AlignRight);

    connect(m_bitmap, qOverload<int>(&QComboBox::currentIndexChanged), this, &BitmapPanel::updateBitmapState);
    connect(m_x, qOverload<int>(&QSpinBox::valueChanged), this, &BitmapPanel::updatePreview);
    connect(m_y, qOverload<int>(&QSpinBox::valueChanged), this, &BitmapPanel::updatePreview);

    refreshBitmaps();
}

void BitmapPanel::updatePreview() const
{
    if (!canDraw()) {
        emit refreshRequested();
        return;
    }

    ControlPanel::updatePreview();
}

void BitmapPanel::updateRange(epd_gfx_canvas_t canvas)
{
    setPointRange(m_x, m_y, canvas);
}

void BitmapPanel::refreshProjectResources()
{
    refreshBitmaps();
}

DrawFunc BitmapPanel::drawFunc() const
{
    BitmapProvider* provider = m_bitmapProvider;

    BitmapDrawRequest request;
    request.fileName = m_bitmap->currentData().toString();
    request.point.x  = static_cast<uint16_t>(m_x->value());
    request.point.y  = static_cast<uint16_t>(m_y->value());

    return [provider, request](epd_gfx_canvas_t canvas) {
        if (!provider) {
            return EPD_ERR_INVALID_ARG;
        }
        return provider->drawBitmap(canvas, request);
    };
}

void BitmapPanel::refreshBitmaps()
{
    const QString currentFileName = m_bitmap->currentData().toString();

    m_bitmap->blockSignals(true);
    m_bitmap->clear();

    if (m_bitmapProvider) {
        const QVector<BitmapResourceInfo> bitmaps = m_bitmapProvider->bitmaps();
        for (const BitmapResourceInfo& bitmap : bitmaps) {
            m_bitmap->addItem(bitmap.displayName, bitmap.fileName);
        }
    }

    if (m_bitmap->count() == 0) {
        m_bitmap->addItem(QStringLiteral("No Bitmaps"), QString());
    } else {
        const int index = m_bitmap->findData(currentFileName);
        m_bitmap->setCurrentIndex(index >= 0 ? index : 0);
    }
    m_bitmap->blockSignals(false);

    updateBitmapState();
}

void BitmapPanel::updateBitmapState()
{
    BitmapResourceInfo info;
    if (m_bitmapProvider && m_bitmapProvider->bitmap(m_bitmap->currentData().toString(), &info)) {
        m_size->setText(QStringLiteral("%1 x %2").arg(info.width).arg(info.height));
    } else {
        m_size->setText(QStringLiteral("-"));
    }

    updateControls();
    updatePreview();
}

void BitmapPanel::updateControls()
{
    const bool enabled = canDraw();

    m_bitmap->setEnabled(enabled);
    m_size->setEnabled(enabled);
    m_x->setEnabled(enabled);
    m_y->setEnabled(enabled);
    m_previewBtn->setEnabled(enabled);
    m_draw->setEnabled(enabled);
}

bool BitmapPanel::canDraw() const
{
    return m_bitmapProvider
        && !m_bitmap->currentData().toString().isEmpty();
}

LEKCO_END_NAMESPACE
