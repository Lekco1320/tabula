/**
 * @file CanvasPreviewer.cpp
 * @brief Tri-color previewer for `epd_canvas`.
 * 
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2025-12-10
 * @license MIT
 */

#include <QPainter>
#include <QVariantAnimation>
#include <QEasingCurve>
#include <fmt/core.h>
#include <epd_core/common.h>
#include <epd_gfx/codec.h>

#include "CanvasPreviewer.hpp"

_LEKCO_BEGIN_NAMESPACE

CanvasPreviewer::CanvasPreviewer(epd_gfx_canvas_config_t config, QWidget* parent)
    : QWidget(parent)
    , m_config(config)
    , m_canvas(nullptr)
    , m_angleFrom(0.0)
    , m_angleTo(config.rotation * 90.0)
    , m_angleCurrent(config.rotation * 90.0)
{
    epd_err_t status = epd_gfx_canvas_create(&config, &m_canvas);
    if (status != EPD_OK) {
        throw std::runtime_error("CanvasPreviewer initialization failed");
    }

    m_baseImage = QImage(m_config.width, m_config.height, QImage::Format_RGB888);
    m_baseImage.fill(Qt::white);

    qreal diag = ceil(sqrt(m_config.width * m_config.width + m_config.height * m_config.height)) + 2 * 8;
    setMinimumSize(QSize(diag, diag));
}

CanvasPreviewer::~CanvasPreviewer()
{
    if (m_canvas) {
        epd_gfx_canvas_destroy(m_canvas);
        m_canvas = nullptr;
    }
}

void CanvasPreviewer::loadNativeBuffer(const uint8_t* data, uint32_t size)
{
    epd_err_t status = epd_gfx_canvas_load_native(m_canvas, data, size);
    if (status != EPD_OK) {
        throw std::runtime_error(fmt::format("CanvasPreviewer load planes buffer failed: {}",
            epd_err_to_str(status)));
    }
    rebuildImage();
    update();
}

void CanvasPreviewer::loadPlanesBuffer(const uint8_t* pwht, const uint8_t* pred, uint32_t size)
{
    epd_err_t status = epd_gfx_canvas_load_planes(m_canvas, pwht, pred, size);
    if (status != EPD_OK) {
        throw std::runtime_error(fmt::format("CanvasPreviewer load planes buffer failed: {}",
            epd_err_to_str(status)));
    }
    rebuildImage();
    update();
}

void CanvasPreviewer::setRotation(epd_gfx_rotation_t rotation)
{
    epd_gfx_rotation_t canvasRotation = epd_gfx_canvas_get_rotation(m_canvas);
    if (rotation == canvasRotation) {
        return;
    }

    epd_err_t status = epd_gfx_canvas_set_rotation(m_canvas, rotation);
    if (status != EPD_OK) {
        throw std::runtime_error(fmt::format("CanvasPreviewer set rotation failed: {}",
            epd_err_to_str(status)));
    }

    m_config.rotation = rotation;
    startRotationAnimation(rotation);
}

epd_gfx_canvas_t CanvasPreviewer::getCanvas() const
{
    return m_canvas;
}

void CanvasPreviewer::refresh()
{
    if (!m_canvas) {
        return;
    }

    rebuildImage();
    update();
}

void CanvasPreviewer::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.fillRect(rect(), Qt::white);

    QRectF content = rect().adjusted(4, 4, -4, -4);
    // If no image yet, draw a placeholder inside the border.
    if (m_baseImage.isNull()) {
        painter.fillRect(content, Qt::lightGray);
        return;
    }

    painter.save();
    painter.translate(content.center());
    painter.rotate(m_angleCurrent);

    QSizeF nativeSize = m_baseImage.size();
    QRectF target(QPointF(-nativeSize.width() / 2.0, -nativeSize.height() / 2.0), nativeSize);
    painter.drawImage(target, m_baseImage);
    QPen pen(Qt::black);
    pen.setWidth(2);
    painter.setPen(pen);
    painter.drawRect(target);

    painter.restore();
}

void CanvasPreviewer::startRotationAnimation(epd_gfx_rotation_t target)
{
    m_angleFrom = m_angleCurrent;
    m_angleTo   = target * 90.0;

    auto* anim = new QVariantAnimation(this);
    anim->setDuration(abs(m_angleTo - m_angleFrom) / 90 * 200);
    anim->setStartValue(0.0);
    anim->setEndValue(1.0);
    anim->setEasingCurve(QEasingCurve::InOutCubic);
    connect(anim, &QVariantAnimation::valueChanged, this, [this](const QVariant& v) {
        qreal t = v.toReal();
        m_angleCurrent = m_angleFrom + (m_angleTo - m_angleFrom) * t;
        update();
    });
    connect(anim, &QVariantAnimation::finished, anim, &QObject::deleteLater);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void CanvasPreviewer::rebuildImage()
{
    epd_gfx_frame_view_sink_t sink = {
        .context    = this,
        .flush_impl = &CanvasPreviewer::flushImpl,
    };

    epd_err_t status = epd_gfx_canvas_flush(m_canvas, &sink);
    if (status != EPD_OK) {
        throw std::runtime_error(fmt::format("CanvasPreviewer rebuild image failed: {}",
            epd_err_to_str(status)));
    }
}

epd_err_t CanvasPreviewer::rebuildImageFromNative(const uint8_t* data, uint32_t size)
{
    if (!data) {
        return EPD_ERR_INVALID_ARG;
    }

    const uint16_t height = m_config.height;
    const uint32_t stride = epd_gfx_native_stride(m_config.width);
    const uint32_t length = stride * m_config.height;
    if (size != length) {
        return EPD_ERR_INVALID_SIZE;
    }

    QVector<epd_gfx_color_t> colors(m_config.width);
    for (uint16_t y = 0; y < height; ++y) {
        const uint8_t* row = data + stride * y;
        epd_err_t status = epd_gfx_native_to_color_buffer(row, m_config.width, colors.data());
        if (status != EPD_OK) {
            return status;
        }
        for (uint16_t x = 0; x < m_config.width; ++x) {
            epd_gfx_color_t color = colors[x];
            m_baseImage.setPixelColor(x, y, color == EPD_GFX_WHITE ? Qt::white :
                                            color == EPD_GFX_BLACK ? Qt::black : Qt::red);
        }
    }
    return EPD_OK;
}

epd_err_t CanvasPreviewer::rebuildImageFromPlanes(const uint8_t* pwht, const uint8_t* pred, uint32_t size)
{
    if (!pwht || !pred) {
        return EPD_ERR_INVALID_ARG;
    }

    const uint16_t height = m_config.height;
    const uint32_t stride = epd_gfx_planes_stride(m_config.width);
    const uint32_t length = stride * m_config.height;
    if (size != length) {
        return EPD_ERR_INVALID_SIZE;
    }

    QVector<epd_gfx_color_t> colors(m_config.width);
    for (uint16_t y = 0; y < height; ++y) {
        const uint8_t* row_wht = pwht + stride * y;
        const uint8_t* row_red = pred + stride * y;
        epd_err_t status = epd_gfx_planes_to_color_buffer(row_wht, row_red, m_config.width, colors.data());
        if (status != EPD_OK) {
            return status;
        }
        for (uint16_t x = 0; x < m_config.width; ++x) {
            epd_gfx_color_t color = colors[x];
            m_baseImage.setPixelColor(x, y, color == EPD_GFX_WHITE ? Qt::white :
                                            color == EPD_GFX_BLACK ? Qt::black : Qt::red);
        }
    }
    return EPD_OK;
}

epd_err_t CanvasPreviewer::flushImpl(void* ctx, const epd_gfx_frame_view_t* view)
{
    CanvasPreviewer* previewer = (CanvasPreviewer*)ctx;
    if (!previewer || !view) {
        return EPD_ERR_INVALID_ARG;
    }

    switch (previewer->m_config.format)
    {
    case EPD_GFX_FORMAT_NATIVE:
        return previewer->rebuildImageFromNative(view->buf_native, view->stride * view->height);
    
    case EPD_GFX_FORMAT_PLANES:
        return previewer->rebuildImageFromPlanes(view->buf_wht, view->buf_red, view->stride * view->height);

    default:
        return EPD_ERR_INVALID_ARG;
    }
}

_LEKCO_END_NAMESPACE
