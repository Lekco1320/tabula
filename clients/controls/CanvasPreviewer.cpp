/**
 * @file CanvasPreviewer.cpp
 * @brief Tri-color previewer for `epd_canvas_t`.
 * 
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2025-12-10
 * @license MIT
 */

#include <cmath>
#include <algorithm>
#include <QPainter>
#include <QVariantAnimation>
#include <QEasingCurve>
#include <QMouseEvent>
#include <QCursor>
#include <QFontMetrics>
#include <QTransform>
#include <QEnterEvent>
#include <epd_core/common.h>
#include <epd_gfx/codec.h>

#include "Utils.hpp"
#include "CanvasPreviewer.hpp"

BEGIN_NAMESPACE()

constexpr qreal kContentMargin = 4.0;
constexpr int   kOverlayMargin = 8;
constexpr int   kPreviewSize   = 90;
constexpr qreal kPreviewZoom   = 2.0;
constexpr int   kBorderWidth   = 2;
constexpr int   kTextPadH      = 6;
constexpr int   kTextPadV      = 2;

inline QString colorShortName(const QColor& c)
{
    return (c == Qt::black) ? QStringLiteral("B") :
           (c == Qt::white) ? QStringLiteral("W") :
                              QStringLiteral("R");
}

inline QRectF contentRect(const QWidget* w)
{
    return w->rect().adjusted(kContentMargin, kContentMargin, -kContentMargin, -kContentMargin);
}

QRectF computeTargetRect(const QImage& img)
{
    const QSizeF size = img.size();
    return QRectF(QPointF(-size.width() / 2.0, -size.height() / 2.0), size);
}

QRectF computeSourceRect(const QPointF& imagePos, const QSize& imageSize)
{
    const qreal srcW  = kPreviewSize / kPreviewZoom;
    const qreal srcH  = kPreviewSize / kPreviewZoom;
    const qreal halfW = srcW / 2.0;
    const qreal halfH = srcH / 2.0;
    const qreal srcX  = std::clamp(imagePos.x() - halfW, 0.0, imageSize.width() - srcW);
    const qreal srcY  = std::clamp(imagePos.y() - halfH, 0.0, imageSize.height() - srcH);
    return QRectF(srcX, srcY, srcW, srcH);
}

void drawCrosshair(QPainter& p, const QRectF& rect, const QPointF& point, const QPen& pen)
{
    p.save();
    p.setPen(pen);
    p.drawLine(QPointF(point.x(), rect.top()), QPointF(point.x(), rect.bottom()));
    p.drawLine(QPointF(rect.left(), point.y()), QPointF(rect.right(), point.y()));
    p.restore();
}

struct OverlayRects {
    QRectF overlay;
    QRectF preview;
    QRectF text;
    QRectF textBg;
};

OverlayRects makeOverlayRects(const QRectF& content, int textHeight)
{
    const int overlayWidth  = kPreviewSize;
    const int overlayHeight = kPreviewSize + textHeight + kTextPadV * 2;

    OverlayRects r;
    r.overlay = QRectF(content.left() + kOverlayMargin,
                       content.bottom() - overlayHeight - kOverlayMargin,
                       overlayWidth,
                       overlayHeight);

    r.preview = QRectF(r.overlay.left(),
                       r.overlay.top(),
                       kPreviewSize,
                       kPreviewSize);

    r.text = QRectF(r.overlay.left() + kTextPadH,
                    r.preview.bottom() + kTextPadV,
                    overlayWidth - kTextPadH * 2,
                    textHeight);

    r.textBg = r.text.adjusted(-kTextPadH, -kTextPadV, kTextPadH, kTextPadV);
    return r;
}

END_NAMESPACE

LEKCO_BEGIN_NAMESPACE

CanvasPreviewer::CanvasPreviewer(epd_gfx_canvas_config_t config, QWidget* parent)
    : QWidget(parent)
    , m_config(config)
    , m_canvas(nullptr)
    , m_angleFrom(0.0)
    , m_angleTo(config.rotation * 90.0)
    , m_angleCurrent(config.rotation * 90.0)
    , m_cursor(Cursor::Pointer)
    , m_hasMouse(false)
    , m_lastMousePos(QPointF())
{
    epd_err_t status = epd_gfx_canvas_create(&config, &m_canvas);
    if (status != EPD_OK) {
        throw std::runtime_error("CanvasPreviewer initialization failed");
    }

    epd_gfx_canvas_fill(m_canvas, EPD_GFX_WHITE);
    m_baseImage = QImage(m_config.width, m_config.height, QImage::Format_RGB888);
    m_baseImage.fill(Qt::white);

    qreal diag = ceil(sqrt(m_config.width * m_config.width + m_config.height * m_config.height)) + 2 * 8;
    setMinimumSize(QSize(diag, diag));
    setMouseTracking(true);
}

CanvasPreviewer::~CanvasPreviewer()
{
    if (m_canvas) {
        epd_gfx_canvas_destroy(m_canvas);
        m_canvas = nullptr;
    }
}

void CanvasPreviewer::setRotation(epd_gfx_rotation_t rotation)
{
    epd_gfx_rotation_t canvasRotation = epd_gfx_canvas_get_rotation(m_canvas);
    if (rotation == canvasRotation) {
        return;
    }

    epd_err_t status = epd_gfx_canvas_set_rotation(m_canvas, rotation);
    if (status != EPD_OK) {
        throw std::runtime_error(QStringLiteral("CanvasPreviewer set rotation failed: %1")
            .arg(epd_err_to_str(status))
            .toStdString());
    }

    m_config.rotation = rotation;
    emit rotationChanged(rotation);

    startRotationAnimation(rotation);
}

epd_gfx_canvas_t CanvasPreviewer::getCanvas() const
{
    return m_canvas;
}

void CanvasPreviewer::setCursor(Cursor mode)
{
    if (m_cursor == mode) {
        return;
    }
    
    m_cursor = mode;
    if (m_cursor == Cursor::Pointer) {
        unsetCursor();
    }
    update();
}

void CanvasPreviewer::setPreviewCanvas(epd_gfx_canvas_t preview)
{
    if (!preview) {
        throw std::runtime_error("Pointer to preview canvas is null");
    }

    rebuildImage(preview);
    update();
}

void CanvasPreviewer::refresh()
{
    if (!m_canvas) {
        return;
    }

    rebuildImage(m_canvas);
    update();
}

void CanvasPreviewer::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    const HoverInfo hover = (m_cursor == Cursor::Inspect) ? currentHoverInfo() : HoverInfo{};

    QPainter painter(this);
    painter.fillRect(rect(), Qt::white);

    const QRectF content = contentRect(this);
    // If no image yet, draw a placeholder inside the border.
    if (m_baseImage.isNull()) {
        painter.fillRect(content, Qt::lightGray);
        return;
    }

    drawCanvasImage(painter, content, hover);

    if (hover.active) {
        drawOverlay(painter, content, hover);
    }
}

void CanvasPreviewer::enterEvent(QEnterEvent* event)
{
    Q_UNUSED(event);
    m_hasMouse = true;
    if (m_cursor == Cursor::Inspect) {
        update();
    }
}

void CanvasPreviewer::leaveEvent(QEvent* event)
{
    Q_UNUSED(event);
    m_hasMouse = false;
    unsetCursor();
    if (m_cursor == Cursor::Inspect) {
        update();
    }
}

void CanvasPreviewer::mouseMoveEvent(QMouseEvent* event)
{
    m_lastMousePos = event->position();
    QPointF imagePos;
    QRectF  targetRect;
    if (m_cursor == Cursor::Inspect) {
        if (mapToImage(m_lastMousePos, imagePos, targetRect)) {
            static_cast<QWidget*>(this)->setCursor(Qt::CrossCursor);
        } else {
            unsetCursor();
        }
        update();
    }
}

bool CanvasPreviewer::mapToImage(const QPointF& widgetPos, QPointF& imagePos, QRectF& targetRect) const
{
    if (m_baseImage.isNull()) {
        return false;
    }

    const QRectF  content = contentRect(this);
    const QPointF local   = widgetPos - content.center();

    QTransform inv;
    inv.rotate(-m_angleCurrent);
    QPointF imgSpace = inv.map(local);

    QSizeF nativeSize = m_baseImage.size();
    targetRect = QRectF(QPointF(-nativeSize.width() / 2.0, -nativeSize.height() / 2.0), nativeSize);
    if (!targetRect.contains(imgSpace)) {
        return false;
    }

    imagePos = QPointF(imgSpace.x() + nativeSize.width() / 2.0,
                       imgSpace.y() + nativeSize.height() / 2.0);
    return true;
}

CanvasPreviewer::HoverInfo CanvasPreviewer::currentHoverInfo() const
{
    HoverInfo info;
    if (!m_hasMouse || m_cursor != Cursor::Inspect) {
        return info;
    }

    QPointF imagePos;
    QRectF  targetRect;
    if (!mapToImage(m_lastMousePos, imagePos, targetRect)) {
        return info;
    }

    const int ix = std::clamp(static_cast<int>(std::floor(imagePos.x())), 0, m_baseImage.width() - 1);
    const int iy = std::clamp(static_cast<int>(std::floor(imagePos.y())), 0, m_baseImage.height() - 1);

    info.active     = true;
    info.widgetPos  = m_lastMousePos;
    info.imagePos   = imagePos;
    info.targetRect = targetRect;
    info.color      = m_baseImage.pixelColor(ix, iy);
    return info;
}

QPoint CanvasPreviewer::mapToLogical(const QPoint& base) const
{
    switch (m_config.rotation)
    {
    case EPD_GFX_ROTATE_0:
        return base;

    case EPD_GFX_ROTATE_90:
        return QPoint(m_config.height - 1 - base.y(), base.x());

    case EPD_GFX_ROTATE_180:
        return QPoint(m_config.width - 1 - base.x(), m_config.height - 1 - base.y());

    case EPD_GFX_ROTATE_270:
        return QPoint(base.y(), m_config.width - 1 - base.x());

    default:
        return base;
    }
}

void CanvasPreviewer::drawCanvasImage(QPainter& painter, const QRectF& content, const HoverInfo& hover) const
{
    painter.save();
    painter.translate(content.center());
    painter.rotate(m_angleCurrent);

    const QRectF target = computeTargetRect(m_baseImage);
    painter.drawImage(target, m_baseImage);

    QPen borderPen(Qt::black);
    borderPen.setWidth(kBorderWidth);
    painter.setPen(borderPen);
    painter.drawRect(target);

    if (hover.active) {
        const QSizeF nativeSize = m_baseImage.size();
        const qreal  relX       = hover.imagePos.x() - nativeSize.width() / 2.0;
        const qreal  relY       = hover.imagePos.y() - nativeSize.height() / 2.0;
        QPen crossPen(QColor(0, 0, 0, 140));
        crossPen.setWidth(1);
        drawCrosshair(painter, target, QPointF(relX, relY), crossPen);
    }

    painter.restore();
}

void CanvasPreviewer::drawOverlay(QPainter& painter, const QRectF& content, const HoverInfo& hover) const
{
    painter.save();
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);

    const QFontMetrics fm(painter.font());
    const OverlayRects r = makeOverlayRects(content, fm.height());

    painter.setPen(QColor("#202020"));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(r.overlay);

    const QRectF srcRect = computeSourceRect(hover.imagePos, m_baseImage.size());
    painter.save();
    painter.translate(r.preview.center());
    painter.rotate(m_angleCurrent);

    const QSizeF targetSize = r.preview.size();
    const QRectF target(QPointF(-targetSize.width() / 2.0, -targetSize.height() / 2.0), targetSize);
    painter.drawImage(target, m_baseImage, srcRect);
    painter.setPen(QColor("#202020"));
    painter.drawRect(target);

    QPen miniCrossPen(QColor("#808080"));
    miniCrossPen.setWidth(1);
    const qreal scaleX = targetSize.width() / srcRect.width();
    const qreal scaleY = targetSize.height() / srcRect.height();
    const qreal cx     = (hover.imagePos.x() - srcRect.left()) * scaleX - targetSize.width() / 2.0;
    const qreal cy     = (hover.imagePos.y() - srcRect.top()) * scaleY - targetSize.height() / 2.0;
    drawCrosshair(painter, target, QPointF(cx, cy), miniCrossPen);
    painter.restore();

    const int ix = std::clamp(static_cast<int>(std::floor(hover.imagePos.x())), 0, m_baseImage.width() - 1);
    const int iy = std::clamp(static_cast<int>(std::floor(hover.imagePos.y())), 0, m_baseImage.height() - 1);
    const QPoint logical = mapToLogical(QPoint(ix, iy));

    const QColor bgColor = hover.color;
    const QColor fgColor = (bgColor == Qt::white) ? Qt::black : Qt::white;
    painter.setPen(QColor("#202020"));
    painter.setBrush(bgColor);
    painter.drawRect(r.textBg);
    painter.setPen(fgColor);

    const QString text = QStringLiteral("(%1, %2) %3")
        .arg(logical.x() + 1)
        .arg(logical.y() + 1)
        .arg(colorShortName(hover.color));
    painter.drawText(r.text, Qt::AlignHCenter | Qt::AlignVCenter, text);

    painter.restore();
}

void CanvasPreviewer::startRotationAnimation(epd_gfx_rotation_t target)
{
    m_angleFrom = m_angleCurrent;
    m_angleTo   = target * 90.0;

    auto* anim = new QVariantAnimation(this);
    int duration = static_cast<int>(std::abs(m_angleTo - m_angleFrom) / 90.0 * 200);
    anim->setDuration(std::max(duration, 1));
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

void CanvasPreviewer::rebuildImage(epd_gfx_canvas_t canvas)
{
    epd_gfx_frame_view_sink_t sink = {
        .context    = this,
        .flush_impl = &CanvasPreviewer::flushImpl,
    };

    epd_err_t status = epd_gfx_canvas_flush(canvas, &sink);
    if (status != EPD_OK) {
        throw std::runtime_error(QStringLiteral("CanvasPreviewer rebuild image failed: %1")
            .arg(epd_err_to_str(status))
            .toStdString());
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
            m_baseImage.setPixelColor(x, y, EpdColorToQColor(color));
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
            m_baseImage.setPixelColor(x, y, EpdColorToQColor(color));
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

LEKCO_END_NAMESPACE
