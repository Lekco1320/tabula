/**
 * @file stream.c
 * @brief Stream helper API implementation.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2026-05-17
 * @license MIT
 */

#include <string.h>
#include <epd_core/stream.h>

bool epd_stream_read_exact(const epd_stream_t* stream, void* dst, size_t size)
{
    if (size == 0U) {
        return true;
    }
    if (!stream || !stream->read || !dst) {
        return false;
    }

    return stream->read(stream->ctx, dst, size) == size;
}

bool epd_stream_write_exact(const epd_stream_t* stream, const void* src, size_t size)
{
    if (size == 0U) {
        return true;
    }
    if (!stream || !stream->write || !src) {
        return false;
    }

    return stream->write(stream->ctx, src, size) == size;
}

bool epd_stream_seek(const epd_stream_t* stream, int64_t offset, epd_seek_whence_t whence)
{
    if (!stream || !stream->seek) {
        return false;
    }

    return stream->seek(stream->ctx, offset, whence);
}

bool epd_stream_close(epd_stream_t* stream)
{
    if (!stream || !stream->ctx || !stream->close) {
        return false;
    }

    bool closed = stream->close(stream->ctx);
    memset(stream, 0, sizeof(*stream));
    return closed;
}
