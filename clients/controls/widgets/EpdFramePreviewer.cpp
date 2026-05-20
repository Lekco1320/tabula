/**
 * @file EpdFramePreviewer.cpp
 * @brief Scrollable tri-color EPD frame preview widget implementation.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2026-05-20
 * @license MIT
 */

#include <algorithm>
#include <cmath>
#include <QEnterEvent>
#include <QEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QResizeEvent>
#include <QScrollArea>
#include <QScrollBar>
#include <QSignalBlocker>
#include <QToolButton>
#include <QTransform>
#include <QVBoxLayout>
#include <QWheelEvent>

#include "controls/Utils.hpp"
#include "controls/widgets/CycleIconButton.hpp"
#include "controls/widgets/EpdFramePreviewer.hpp"

BEGIN_NAMESPACE()

constexpr int kPreviewSize = 90;
constexpr qreal kPreviewZoom = 2.0;
constexpr int kOverlayMargin = 8;
constexpr int kBorderWidth = 1;
const int kZoomStops[] = { 25, 33, 50, 67, 75, 100, 150, 200, 300, 400, 600, 800 };

QString ColorShortName(const QColor& color)
{
    return (color == Qt::black) ? QStringLiteral("B")
        : (color == Qt::red)    ? QStringLiteral("R")
                                : QStringLiteral("W");
}

QRectF SourcePreviewRect(const QPointF& imagePos, const QSize& imageSize)
{
    const qreal srcW  = kPreviewSize / kPreviewZoom;
    const qreal srcH  = kPreviewSize / kPreviewZoom;
    const qreal halfW = srcW / 2.0;
    const qreal halfH = srcH / 2.0;
    const qreal srcX  = std::clamp(imagePos.x() - halfW, 0.0,
        std::max(0.0, imageSize.width() - srcW));
    const qreal srcY  = std::clamp(imagePos.y() - halfH, 0.0,
        std::max(0.0, imageSize.height() - srcH));
    return QRectF(srcX, srcY, srcW, srcH);
}

void DrawCrosshair(QPainter& painter, const QRectF& rect, const QPointF& point,
    const QColor& color)
{
    painter.save();
    painter.setPen(QPen(color, 1));
    painter.drawLine(QPointF(point.x(), rect.top()), QPointF(point.x(), rect.bottom()));
    painter.drawLine(QPointF(rect.left(), point.y()), QPointF(rect.right(), point.y()));
    painter.restore();
}

END_NAMESPACE

LEKCO_BEGIN_NAMESPACE

class PreviewCursorButton
    : public CycleIconButton
{
public:
    explicit PreviewCursorButton(QWidget* parent = nullptr)
        : CycleIconButton(parent)
    {
        setToolTip(QStringLiteral("Pointer"));
        setIcons({
            QStringLiteral(":/common/icons/Pointer.svg"),
            QStringLiteral(":/common/icons/Inspect.svg"),
        });

        connect(this, &CycleIconButton::currentIndexChanged, this, [this](int index) {
            setToolTip(index == 0 ? QStringLiteral("Pointer") : QStringLiteral("Inspect"));
        });
    }
};

QToolButton* CreateZoomButton(QWidget* parent, const QString& iconPath,
    const QString& toolTip)
{
    auto* button = new QToolButton(parent);
    SetupIconToolButton(button);
    button->setIcon(QIcon(iconPath));
    button->setToolTip(toolTip);
    return button;
}

class EpdFramePreviewer::Viewport
    : public QWidget
{
public:
    explicit Viewport(EpdFramePreviewer* owner, QWidget* parent = nullptr)
        : QWidget(parent)
        , m_owner(owner)
    {
        setMouseTracking(true);
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    }

    void setImage(const QImage& image)
    {
        m_image       = image.convertToFormat(QImage::Format_RGB888);
        m_mouseInside = false;
        refreshHover();
        updateSize();
        update();
    }

    void clear()
    {
        m_image = QImage();
        m_mouseInside = false;
        refreshHover();
        updateSize();
        update();
    }

    void setCursorMode(Cursor cursor)
    {
        m_cursor = cursor;
        if (m_cursor == Cursor::Pointer) {
            unsetCursor();
        }
        refreshHover();
        update();
    }

    void setRotation(epd_gfx_rotation_t rotation)
    {
        m_rotation = rotation;
        updateSize();
        refreshHover();
        update();
    }

    void setLogicalSize(uint16_t width, uint16_t height)
    {
        m_logicalWidth  = width;
        m_logicalHeight = height;
        refreshHover();
        update();
    }

    void setZoomPercent(int percent)
    {
        m_zoomPercent = percent;
        updateSize();
        refreshHover();
        update();
    }

    int zoomPercent() const
    {
        return m_zoomPercent;
    }

    bool imagePositionAt(const QPoint& pos, QPointF* outImagePos) const
    {
        return mapWidgetToImage(pos, outImagePos);
    }

protected:
    void paintEvent(QPaintEvent* event) override
    {
        Q_UNUSED(event);

        QPainter painter(this);
        painter.fillRect(rect(), QColor("#f5f5f5"));
        if (m_image.isNull()) {
            painter.setPen(QColor("#808080"));
            painter.drawText(rect(), Qt::AlignCenter, QStringLiteral("No Preview"));
            return;
        }

        painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
        const QImage displayImage = rotatedImage();
        const QRectF target = targetRect();
        painter.drawImage(target, displayImage);
        painter.setPen(QPen(Qt::black, kBorderWidth));
        painter.drawRect(target);

        QPointF imagePos;
        const bool hover = m_cursor == Cursor::Inspect && m_mouseInside
            && mapWidgetToImage(m_lastMousePos.toPoint(), &imagePos);
        if (hover) {
            const QPointF rotatedPos = imageToRotatedPosition(imagePos);
            const QPointF targetPos = target.topLeft()
                + QPointF(rotatedPos.x() * zoomScale(), rotatedPos.y() * zoomScale());
            DrawCrosshair(painter, target, targetPos, QColor(0, 0, 0, 140));
        }
    }

    void enterEvent(QEnterEvent* event) override
    {
        Q_UNUSED(event);
        m_mouseInside = true;
        refreshHover();
    }

    void leaveEvent(QEvent* event) override
    {
        Q_UNUSED(event);
        m_mouseInside = false;
        unsetCursor();
        if (m_owner) {
            m_owner->updateHover(false);
        }
        update();
    }

    void mouseMoveEvent(QMouseEvent* event) override
    {
        m_lastMousePos = event->position();
        refreshHover();
        update();
    }

private:
    qreal zoomScale() const
    {
        return static_cast<qreal>(m_zoomPercent) / 100.0;
    }

    QImage rotatedImage() const
    {
        if (m_image.isNull()) {
            return QImage();
        }

        QTransform transform;
        transform.rotate(static_cast<int>(m_rotation) * 90);
        return m_image.transformed(transform, Qt::FastTransformation);
    }

    QRectF targetRect() const
    {
        const QImage image = rotatedImage();
        const QSizeF size(image.width() * zoomScale(), image.height() * zoomScale());
        return QRectF(QPointF(kOverlayMargin, kOverlayMargin), size);
    }

    void updateSize()
    {
        const QImage image = rotatedImage();
        if (image.isNull()) {
            setMinimumSize(320, 240);
            resize(minimumSize());
            return;
        }

        const QSize size(
            static_cast<int>(std::ceil(image.width() * zoomScale())) + kOverlayMargin * 2,
            static_cast<int>(std::ceil(image.height() * zoomScale())) + kOverlayMargin * 2);
        setMinimumSize(size);
        resize(size);
    }

    bool mapWidgetToImage(const QPoint& pos, QPointF* outImagePos) const
    {
        if (m_image.isNull() || !outImagePos) {
            return false;
        }

        const QRectF target = targetRect();
        if (!target.contains(pos)) {
            return false;
        }

        const QPointF rotatedPos((pos.x() - target.left()) / zoomScale(),
            (pos.y() - target.top()) / zoomScale());
        const QImage rotated = rotatedImage();
        QPointF imagePos;
        switch (m_rotation) {
        case EPD_GFX_ROTATE_90:
            imagePos = QPointF(rotatedPos.y(), m_image.height() - 1 - rotatedPos.x());
            break;

        case EPD_GFX_ROTATE_180:
            imagePos = QPointF(m_image.width() - 1 - rotatedPos.x(),
                m_image.height() - 1 - rotatedPos.y());
            break;

        case EPD_GFX_ROTATE_270:
            imagePos = QPointF(m_image.width() - 1 - rotatedPos.y(), rotatedPos.x());
            break;

        case EPD_GFX_ROTATE_0:
        default:
            imagePos = rotatedPos;
            break;
        }

        if (imagePos.x() < 0.0 || imagePos.y() < 0.0
            || imagePos.x() >= m_image.width() || imagePos.y() >= m_image.height()
            || rotatedPos.x() < 0.0 || rotatedPos.y() < 0.0
            || rotatedPos.x() >= rotated.width() || rotatedPos.y() >= rotated.height()) {
            return false;
        }

        *outImagePos = imagePos;
        return true;
    }

    QPointF imageToRotatedPosition(const QPointF& imagePos) const
    {
        switch (m_rotation) {
        case EPD_GFX_ROTATE_90:
            return QPointF(m_image.height() - 1 - imagePos.y(), imagePos.x());

        case EPD_GFX_ROTATE_180:
            return QPointF(m_image.width() - 1 - imagePos.x(), m_image.height() - 1 - imagePos.y());

        case EPD_GFX_ROTATE_270:
            return QPointF(imagePos.y(), m_image.width() - 1 - imagePos.x());

        case EPD_GFX_ROTATE_0:
        default:
            return imagePos;
        }
    }

    QPoint logicalPoint(const QPoint& imagePoint) const
    {
        switch (m_rotation) {
        case EPD_GFX_ROTATE_90:
            return QPoint(m_logicalWidth - 1 - imagePoint.y(), imagePoint.x());

        case EPD_GFX_ROTATE_180:
            return QPoint(m_logicalWidth - 1 - imagePoint.x(), m_logicalHeight - 1 - imagePoint.y());

        case EPD_GFX_ROTATE_270:
            return QPoint(imagePoint.y(), m_logicalHeight - 1 - imagePoint.x());

        case EPD_GFX_ROTATE_0:
        default:
            return imagePoint;
        }
    }

    void refreshHover()
    {
        QPointF imagePos;
        if (m_cursor == Cursor::Inspect && m_mouseInside
            && mapWidgetToImage(m_lastMousePos.toPoint(), &imagePos)) {
            setCursor(Qt::CrossCursor);
            if (m_owner) {
                const int ix = std::clamp(static_cast<int>(std::floor(imagePos.x())), 0,
                    m_image.width() - 1);
                const int iy = std::clamp(static_cast<int>(std::floor(imagePos.y())), 0,
                    m_image.height() - 1);
                m_owner->updateHover(true, imagePos, logicalPoint(QPoint(ix, iy)),
                    m_image.pixelColor(ix, iy));
            }
        } else {
            unsetCursor();
            if (m_owner) {
                m_owner->updateHover(false);
            }
        }
    }

    EpdFramePreviewer* m_owner        = nullptr;
    QImage             m_image;
    Cursor             m_cursor       = Cursor::Pointer;
    epd_gfx_rotation_t m_rotation     = EPD_GFX_ROTATE_0;
    uint16_t           m_logicalWidth = 0U;
    uint16_t           m_logicalHeight = 0U;
    int                m_zoomPercent  = 100;
    bool               m_mouseInside  = false;
    QPointF            m_lastMousePos;
};

class EpdFramePreviewer::Overlay
    : public QWidget
{
public:
    explicit Overlay(EpdFramePreviewer* owner)
        : QWidget(owner)
        , m_owner(owner)
    {
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setAttribute(Qt::WA_TranslucentBackground);
    }

protected:
    void paintEvent(QPaintEvent* event) override
    {
        Q_UNUSED(event);

        if (!m_owner || !m_owner->m_hoverActive || m_owner->m_image.isNull()) {
            return;
        }

        const int overlayHeight = kPreviewSize + 24;
        const qreal overlayTop = std::max<qreal>(kOverlayMargin,
            height() - overlayHeight - kOverlayMargin);
        const QRectF overlay(kOverlayMargin, overlayTop, kPreviewSize, overlayHeight);
        const QRectF preview(overlay.left(), overlay.top(), kPreviewSize, kPreviewSize);
        const QRectF text(overlay.left(), preview.bottom(), kPreviewSize, 24);

        QPainter painter(this);
        painter.save();
        painter.setPen(QColor("#202020"));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(overlay);

        const QRectF src = SourcePreviewRect(m_owner->m_hoverImagePos, m_owner->m_image.size());
        painter.drawImage(preview, m_owner->m_image, src);
        painter.drawRect(preview);

        const qreal cx = preview.left()
            + (m_owner->m_hoverImagePos.x() - src.left()) * preview.width() / src.width();
        const qreal cy = preview.top()
            + (m_owner->m_hoverImagePos.y() - src.top()) * preview.height() / src.height();
        DrawCrosshair(painter, preview, QPointF(cx, cy), QColor("#808080"));

        const QColor fg = (m_owner->m_hoverColor == Qt::white) ? Qt::black : Qt::white;
        painter.fillRect(text.adjusted(0, 0, -1, -1), m_owner->m_hoverColor);
        painter.setPen(fg);

        const QString label = QStringLiteral("(%1, %2) %3")
            .arg(m_owner->m_hoverLogicalPoint.x() + 1)
            .arg(m_owner->m_hoverLogicalPoint.y() + 1)
            .arg(ColorShortName(m_owner->m_hoverColor));
        painter.drawText(text, Qt::AlignCenter, label);
        painter.restore();
    }

private:
    EpdFramePreviewer* m_owner = nullptr;
};

EpdFramePreviewer::EpdFramePreviewer(QWidget* parent)
    : QWidget(parent)
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto* toolbar = new QFrame(this);
    toolbar->setFrameShape(QFrame::NoFrame);
    auto* toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(8, 6, 8, 6);
    toolbarLayout->setSpacing(6);

    m_cursorButton = new PreviewCursorButton(toolbar);
    toolbarLayout->addWidget(m_cursorButton);

    m_zoomOutButton = CreateZoomButton(toolbar,
        QStringLiteral(":/common/icons/ZoomOut.svg"), QStringLiteral("Zoom out"));
    toolbarLayout->addWidget(m_zoomOutButton);

    m_zoomLabel = new QLabel(toolbar);
    m_zoomLabel->setMinimumWidth(46);
    m_zoomLabel->setAlignment(Qt::AlignCenter);
    toolbarLayout->addWidget(m_zoomLabel);

    m_zoomInButton = CreateZoomButton(toolbar,
        QStringLiteral(":/common/icons/ZoomIn.svg"), QStringLiteral("Zoom in"));
    toolbarLayout->addWidget(m_zoomInButton);

    toolbarLayout->addStretch(1);

    m_viewport = new Viewport(this);
    m_viewport->setZoomPercent(kZoomStops[m_zoomIndex]);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidget(m_viewport);
    m_scrollArea->setWidgetResizable(false);
    m_scrollArea->setAlignment(Qt::AlignCenter);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_viewport->installEventFilter(this);
    m_scrollArea->viewport()->installEventFilter(this);

    root->addWidget(toolbar);
    root->addWidget(m_scrollArea, 1);

    m_overlay = new Overlay(this);
    m_overlay->setGeometry(rect());
    m_overlay->raise();

    updateZoomLabel();

    connect(m_cursorButton, &CycleIconButton::currentIndexChanged, this, &EpdFramePreviewer::setCursorModeIndex);
    connect(m_zoomOutButton, &QToolButton::clicked, this, &EpdFramePreviewer::zoomOut);
    connect(m_zoomInButton, &QToolButton::clicked, this, &EpdFramePreviewer::zoomIn);
}

bool EpdFramePreviewer::setFrameView(const epd_gfx_frame_view_t* view)
{
    QImage image;
    if (!FrameViewToQImage(view, &image)) {
        return false;
    }

    setImage(image);
    return true;
}

void EpdFramePreviewer::setImage(const QImage& image)
{
    m_image = image.convertToFormat(QImage::Format_RGB888);
    m_viewport->setImage(m_image);
    m_viewport->setLogicalSize(static_cast<uint16_t>(image.width()),
        static_cast<uint16_t>(image.height()));
}

void EpdFramePreviewer::clear()
{
    m_image = QImage();
    m_viewport->clear();
}

void EpdFramePreviewer::setDisplayRotation(epd_gfx_rotation_t rotation)
{
    m_viewport->setRotation(rotation);
}

void EpdFramePreviewer::setLogicalSize(uint16_t width, uint16_t height)
{
    m_viewport->setLogicalSize(width, height);
}

bool EpdFramePreviewer::eventFilter(QObject* watched, QEvent* event)
{
    if ((watched == m_viewport || watched == m_scrollArea->viewport())
        && event->type() == QEvent::Wheel) {
        auto* wheel = static_cast<QWheelEvent*>(event);
        if (wheel->modifiers() & Qt::ControlModifier) {
            const int direction = wheel->angleDelta().y() > 0 ? 1 : -1;
            const QPoint anchor = watched == m_viewport
                ? wheel->position().toPoint()
                : m_viewport->mapFrom(m_scrollArea->viewport(), wheel->position().toPoint());
            stepZoom(direction, &anchor);
            return true;
        }
    }

    return QWidget::eventFilter(watched, event);
}

void EpdFramePreviewer::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    if (m_overlay) {
        m_overlay->setGeometry(rect());
        m_overlay->raise();
    }
}

void EpdFramePreviewer::setCursorMode(Cursor cursor)
{
    const int index = cursor == Cursor::Inspect ? 1 : 0;
    if (m_cursorButton->currentIndex() != index) {
        const QSignalBlocker blocker(m_cursorButton);
        m_cursorButton->setCurrentIndex(index);
    }
    m_cursorButton->setToolTip(cursor == Cursor::Inspect
        ? QStringLiteral("Inspect")
        : QStringLiteral("Pointer"));
    m_viewport->setCursorMode(cursor);
}

void EpdFramePreviewer::setCursorModeIndex(int index)
{
    setCursorMode(index == 0 ? Cursor::Pointer : Cursor::Inspect);
}

void EpdFramePreviewer::zoomIn()
{
    stepZoom(1);
}

void EpdFramePreviewer::zoomOut()
{
    stepZoom(-1);
}

void EpdFramePreviewer::stepZoom(int direction, const QPoint* anchor)
{
    constexpr int zoomStopCount = static_cast<int>(sizeof(kZoomStops) / sizeof(kZoomStops[0]));
    setZoomIndex(std::clamp(m_zoomIndex + direction, 0,
        zoomStopCount - 1), anchor);
}

void EpdFramePreviewer::setZoomIndex(int index, const QPoint* anchor)
{
    constexpr int zoomStopCount = static_cast<int>(sizeof(kZoomStops) / sizeof(kZoomStops[0]));
    if (index < 0 || index >= zoomStopCount || index == m_zoomIndex) {
        return;
    }

    QPointF anchorImage;
    QPoint  viewportPos;
    const bool keepAnchor = anchor != nullptr
        && m_viewport->imagePositionAt(*anchor, &anchorImage);
    if (keepAnchor) {
        viewportPos = *anchor;
    }

    m_zoomIndex = index;
    m_viewport->setZoomPercent(kZoomStops[m_zoomIndex]);
    updateZoomLabel();

    if (keepAnchor) {
        const qreal scale = static_cast<qreal>(kZoomStops[m_zoomIndex]) / 100.0;
        m_scrollArea->horizontalScrollBar()->setValue(
            static_cast<int>(anchorImage.x() * scale - viewportPos.x()));
        m_scrollArea->verticalScrollBar()->setValue(
            static_cast<int>(anchorImage.y() * scale - viewportPos.y()));
    }
}

void EpdFramePreviewer::updateZoomLabel()
{
    m_zoomLabel->setText(QStringLiteral("%1%").arg(kZoomStops[m_zoomIndex]));
}

void EpdFramePreviewer::updateHover(bool active, const QPointF& imagePos,
    const QPoint& logicalPoint, const QColor& color)
{
    m_hoverActive       = active;
    m_hoverImagePos     = imagePos;
    m_hoverLogicalPoint = logicalPoint;
    m_hoverColor        = color;
    if (m_overlay) {
        m_overlay->update();
    }
}

LEKCO_END_NAMESPACE
