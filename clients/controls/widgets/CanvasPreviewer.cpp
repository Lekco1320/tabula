/**
 * @file CanvasPreviewer.cpp
 * @brief Tri-color previewer for `epd_canvas_t`.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2025-12-10
 * @license MIT
 */

#include <QVBoxLayout>
#include <epd_core/common.h>

#include "controls/widgets/CanvasPreviewer.hpp"
#include "controls/widgets/EpdFramePreviewer.hpp"

LEKCO_BEGIN_NAMESPACE

CanvasPreviewer::CanvasPreviewer(epd_gfx_canvas_config_t config, QWidget* parent)
    : QWidget(parent)
    , m_config(config)
{
    epd_gfx_canvas_create(&config, &m_canvas);
    epd_gfx_canvas_fill(m_canvas, EPD_GFX_WHITE);

    m_framePreviewer = new EpdFramePreviewer(this);
    m_framePreviewer->setDisplayRotation(m_config.rotation);
    m_framePreviewer->setLogicalSize(m_config.width, m_config.height);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_framePreviewer, 1);

    refresh();
}

CanvasPreviewer::~CanvasPreviewer()
{
    if (m_canvas) {
        epd_gfx_canvas_destroy(m_canvas);
        m_canvas = nullptr;
    }
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

    rebuildImage(m_canvas);
}

void CanvasPreviewer::setRotation(epd_gfx_rotation_t rotation)
{
    if (!m_canvas || rotation == epd_gfx_canvas_get_rotation(m_canvas)) {
        return;
    }

    const epd_err_t status = epd_gfx_canvas_set_rotation(m_canvas, rotation);
    if (status != EPD_OK) {
        emit errorOccurred(QStringLiteral("CanvasPreviewer set rotation failed: %1")
            .arg(epd_err_to_str(status)));
        return;
    }

    m_config.rotation = rotation;
    m_framePreviewer->setDisplayRotation(rotation);
    m_framePreviewer->setLogicalSize(epd_gfx_canvas_get_logical_width(m_canvas),
        epd_gfx_canvas_get_logical_height(m_canvas));
    emit rotationChanged(rotation);
}

void CanvasPreviewer::drawCanvas(const DrawFunc& drawFunc)
{
    const epd_err_t status = drawFunc(m_canvas);
    if (status != EPD_OK) {
        emit errorOccurred(epd_err_to_str(status));
        return;
    }

    rebuildImage(m_canvas);
}

void CanvasPreviewer::drawPreview(const DrawFunc& drawFunc)
{
    epd_gfx_canvas_t cloned = nullptr;
    epd_err_t status = epd_gfx_canvas_clone(m_canvas, &cloned);
    if (status != EPD_OK) {
        emit errorOccurred(QStringLiteral("Pointer to preview canvas is null"));
        return;
    }

    status = drawFunc(cloned);
    if (status != EPD_OK) {
        epd_gfx_canvas_destroy(cloned);
        emit errorOccurred(epd_err_to_str(status));
        return;
    }

    rebuildImage(cloned);
    epd_gfx_canvas_destroy(cloned);
}

void CanvasPreviewer::rebuildImage(epd_gfx_canvas_t canvas)
{
    epd_gfx_frame_view_sink_t sink {};
    sink.context    = this;
    sink.flush_impl = &CanvasPreviewer::flushImpl;

    const epd_err_t status = epd_gfx_canvas_flush(canvas, &sink);
    if (status != EPD_OK) {
        emit errorOccurred(QStringLiteral("CanvasPreviewer rebuild image failed: %1")
            .arg(epd_err_to_str(status)));
    }
}

epd_err_t CanvasPreviewer::flushImpl(void* ctx, const epd_gfx_frame_view_t* view)
{
    CanvasPreviewer* previewer = static_cast<CanvasPreviewer*>(ctx);
    if (!previewer || !view) {
        return EPD_ERR_INVALID_ARG;
    }

    if (!previewer->m_framePreviewer->setFrameView(view)) {
        return EPD_ERR_INVALID_RESPONSE;
    }

    previewer->m_framePreviewer->setLogicalSize(
        epd_gfx_canvas_get_logical_width(previewer->m_canvas),
        epd_gfx_canvas_get_logical_height(previewer->m_canvas));
    return EPD_OK;
}

LEKCO_END_NAMESPACE
