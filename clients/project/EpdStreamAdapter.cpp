/**
 * @file EpdStreamAdapter.cpp
 * @brief Qt IO adapter implementation for epd streams.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2026-05-13
 * @license MIT
 */

#include <QtGlobal>

#include "project/EpdStreamAdapter.hpp"

LEKCO_BEGIN_NAMESPACE

EpdStreamAdapter::EpdStreamAdapter(QIODevice* device)
    : m_device(device)
{
    m_stream.ctx   = this;
    m_stream.read  = &EpdStreamAdapter::read;
    m_stream.write = &EpdStreamAdapter::write;
    m_stream.seek  = &EpdStreamAdapter::seek;
    m_stream.tell  = &EpdStreamAdapter::tell;
    m_stream.size  = &EpdStreamAdapter::size;
    m_stream.eof   = &EpdStreamAdapter::eof;
    m_stream.error = &EpdStreamAdapter::error;
}

epd_stream_t* EpdStreamAdapter::stream()
{
    return &m_stream;
}

const epd_stream_t* EpdStreamAdapter::stream() const
{
    return &m_stream;
}

size_t EpdStreamAdapter::read(void* ctx, void* data, size_t size)
{
    auto* adapter = static_cast<EpdStreamAdapter*>(ctx);
    if (!adapter->m_device) {
        return 0U;
    }

    const qint64 readBytes = adapter->m_device->read(static_cast<char*>(data), static_cast<qint64>(size));
    return readBytes < 0 ? 0U : static_cast<size_t>(readBytes);
}

size_t EpdStreamAdapter::write(void* ctx, const void* data, size_t size)
{
    auto* adapter = static_cast<EpdStreamAdapter*>(ctx);
    if (!adapter->m_device) {
        return 0U;
    }

    const qint64 written = adapter->m_device->write(static_cast<const char*>(data), static_cast<qint64>(size));
    return written < 0 ? 0U : static_cast<size_t>(written);
}

bool EpdStreamAdapter::seek(void* ctx, int64_t offset, epd_seek_whence_t whence)
{
    auto* adapter = static_cast<EpdStreamAdapter*>(ctx);
    if (!adapter->m_device) {
        return false;
    }

    qint64 target = static_cast<qint64>(offset);
    if (whence == EPD_SEEK_CUR) {
        target += adapter->m_device->pos();
    } else if (whence == EPD_SEEK_END) {
        target += adapter->m_device->size();
    }

    return target >= 0 && adapter->m_device->seek(target);
}

int64_t EpdStreamAdapter::tell(void* ctx)
{
    auto* adapter = static_cast<EpdStreamAdapter*>(ctx);
    return adapter->m_device ? static_cast<int64_t>(adapter->m_device->pos()) : -1;
}

int64_t EpdStreamAdapter::size(void* ctx)
{
    auto* adapter = static_cast<EpdStreamAdapter*>(ctx);
    return adapter->m_device ? static_cast<int64_t>(adapter->m_device->size()) : -1;
}

bool EpdStreamAdapter::eof(void* ctx)
{
    auto* adapter = static_cast<EpdStreamAdapter*>(ctx);
    return adapter->m_device ? adapter->m_device->atEnd() : true;
}

int32_t EpdStreamAdapter::error(void* ctx)
{
    Q_UNUSED(ctx)
    return 0;
}

LEKCO_END_NAMESPACE
