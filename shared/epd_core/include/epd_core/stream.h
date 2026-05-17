/**
 * @file stream.h
 * @brief Stream abstraction for file-like access.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2025-12-23
 * @license MIT
 */

#pragma once

#ifndef _EPD_CORE_STREAM_H_
#define _EPD_CORE_STREAM_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    EPD_SEEK_SET = 0,
    EPD_SEEK_CUR = 1,
    EPD_SEEK_END = 2,
} epd_seek_whence_t;

typedef size_t  (*epd_stream_read_fn) (void*, void*, size_t);
typedef size_t  (*epd_stream_write_fn)(void*, const void*, size_t);
typedef bool    (*epd_stream_seek_fn) (void*, int64_t, epd_seek_whence_t);
typedef int64_t (*epd_stream_tell_fn) (void*);
typedef int64_t (*epd_stream_size_fn) (void*);
typedef bool    (*epd_stream_eof_fn)  (void*);
typedef int32_t (*epd_stream_error_fn)(void*);
typedef bool    (*epd_stream_close_fn)(void*);

typedef struct {
    void*               ctx;
    epd_stream_read_fn  read;
    epd_stream_write_fn write;
    epd_stream_seek_fn  seek;
    epd_stream_tell_fn  tell;
    epd_stream_size_fn  size;
    epd_stream_eof_fn   eof;
    epd_stream_error_fn error;
    epd_stream_close_fn close;
} epd_stream_t;

/**
 * @brief Read exactly `size` bytes from a stream.
 *
 * @param stream Source stream.
 * @param dst Destination buffer. May be NULL when `size` is 0.
 * @param size Number of bytes to read.
 * @return true if exactly `size` bytes were read, otherwise false.
 */
bool epd_stream_read_exact(const epd_stream_t* stream, void* dst, size_t size);

/**
 * @brief Write exactly `size` bytes to a stream.
 *
 * @param stream Destination stream.
 * @param src Source buffer. May be NULL when `size` is 0.
 * @param size Number of bytes to write.
 * @return true if exactly `size` bytes were written, otherwise false.
 */
bool epd_stream_write_exact(const epd_stream_t* stream, const void* src, size_t size);

/**
 * @brief Seek to a stream position.
 *
 * @param stream Stream to seek.
 * @param offset Seek offset.
 * @param whence Seek origin.
 * @return true on success, otherwise false.
 */
bool epd_stream_seek(const epd_stream_t* stream, int64_t offset, epd_seek_whence_t whence);

/**
 * @brief Close a stream and clear its callback table.
 *
 * @param stream Stream to close.
 * @return true on success, otherwise false.
 */
bool epd_stream_close(epd_stream_t* stream);

#ifdef __cplusplus
}
#endif

#endif // !_EPD_CORE_STREAM_H_
