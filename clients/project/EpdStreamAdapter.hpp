/**
 * @file EpdStreamAdapter.hpp
 * @brief Qt IO adapter for epd streams.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2026-05-13
 * @license MIT
 */

#pragma once

#ifndef _EPDSTREAMADAPTER_HPP_
#define _EPDSTREAMADAPTER_HPP_

#include <stddef.h>
#include <stdint.h>
#include <QIODevice>
#include <epd_core/stream.h>

#include "common/Common.h"

LEKCO_BEGIN_NAMESPACE

class EpdStreamAdapter
{
public:
    explicit EpdStreamAdapter(QIODevice* device);

    epd_stream_t* stream();
    const epd_stream_t* stream() const;

private:
    static size_t read(void* ctx, void* data, size_t size);
    static size_t write(void* ctx, const void* data, size_t size);
    static bool seek(void* ctx, int64_t offset, epd_seek_whence_t whence);
    static int64_t tell(void* ctx);
    static int64_t size(void* ctx);
    static bool eof(void* ctx);
    static int32_t error(void* ctx);

    QIODevice*   m_device = nullptr;
    epd_stream_t m_stream = {};
};

LEKCO_END_NAMESPACE

#endif // !_EPDSTREAMADAPTER_HPP_
