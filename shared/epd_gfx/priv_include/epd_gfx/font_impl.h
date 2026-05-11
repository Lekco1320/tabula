/**
 * @file font_impl.h
 * @brief Definition for font implementation.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2025-12-22
 * @license MIT
 */

#pragma once

#ifndef _EPD_GFX_FONT_IMPL_H_
#define _EPD_GFX_FONT_IMPL_H_

#include <stdint.h>

#include "epd_core/common.h"
#include "epd_core/stream.h"
#include "epd_gfx/glyph_impl.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EPD_GFX_EGF_MAGIC "EGF1"

#define EPD_GFX_EGF_MAGIC_BYTES       4U
#define EPD_GFX_EGF_HEADER_BYTES      24U
#define EPD_GFX_EGF_SIZE_TABLE_OFFSET EPD_GFX_EGF_HEADER_BYTES
#define EPD_GFX_EGF_SIZE_RECORD_BYTES 20U
#define EPD_GFX_EGF_GLYPH_INDEX_BYTES 18U

typedef struct {
    char     magic[4];
    uint64_t hash;
    uint32_t size_count;
    uint32_t glyph_count;
    uint32_t data_count;
} epd_gfx_egf_header_t;

typedef struct {
    uint16_t size;
    int16_t  ascent;
    int16_t  descent;
    int16_t  line_height;
    uint32_t glyph_count;
    uint32_t glyph_index_offset;
    uint32_t glyph_data_offset;
} epd_gfx_egf_size_record_t;

typedef struct {
    uint32_t codepoint;
    uint16_t width;
    uint16_t height;
    int16_t  xoffset;
    int16_t  yoffset;
    int16_t  advance;
    uint32_t data_offset;
} epd_gfx_egf_glyph_index_t;

static EPD_INLINE bool epd_gfx_egf_stream_read_exact(const epd_stream_t* stream, void* dst, size_t size)
{
    return stream->read(stream->ctx, dst, size) == size;
}

static EPD_INLINE bool epd_gfx_egf_check_magic(const epd_gfx_egf_header_t* header)
{
    return header->magic[0] == EPD_GFX_EGF_MAGIC[0] &&
           header->magic[1] == EPD_GFX_EGF_MAGIC[1] &&
           header->magic[2] == EPD_GFX_EGF_MAGIC[2] &&
           header->magic[3] == EPD_GFX_EGF_MAGIC[3];
}

static EPD_INLINE bool epd_gfx_egf_read_header(const epd_stream_t* stream, epd_gfx_egf_header_t* header)
{
    return epd_gfx_egf_stream_read_exact(stream, header->magic, sizeof(header->magic)) &&
           epd_gfx_egf_stream_read_exact(stream, &header->hash, sizeof(header->hash)) &&
           epd_gfx_egf_stream_read_exact(stream, &header->size_count, sizeof(header->size_count)) &&
           epd_gfx_egf_stream_read_exact(stream, &header->glyph_count, sizeof(header->glyph_count)) &&
           epd_gfx_egf_stream_read_exact(stream, &header->data_count, sizeof(header->data_count));
}

static EPD_INLINE bool epd_gfx_egf_read_size_record(const epd_stream_t* stream,
    epd_gfx_egf_size_record_t* record)
{
    return epd_gfx_egf_stream_read_exact(stream, &record->size, sizeof(record->size)) &&
           epd_gfx_egf_stream_read_exact(stream, &record->ascent, sizeof(record->ascent)) &&
           epd_gfx_egf_stream_read_exact(stream, &record->descent, sizeof(record->descent)) &&
           epd_gfx_egf_stream_read_exact(stream, &record->line_height, sizeof(record->line_height)) &&
           epd_gfx_egf_stream_read_exact(stream, &record->glyph_count, sizeof(record->glyph_count)) &&
           epd_gfx_egf_stream_read_exact(stream, &record->glyph_index_offset, sizeof(record->glyph_index_offset)) &&
           epd_gfx_egf_stream_read_exact(stream, &record->glyph_data_offset, sizeof(record->glyph_data_offset));
}

static EPD_INLINE bool epd_gfx_egf_read_glyph_index(const epd_stream_t* stream,
    epd_gfx_egf_glyph_index_t* record)
{
    return epd_gfx_egf_stream_read_exact(stream, &record->codepoint, sizeof(record->codepoint)) &&
           epd_gfx_egf_stream_read_exact(stream, &record->width, sizeof(record->width)) &&
           epd_gfx_egf_stream_read_exact(stream, &record->height, sizeof(record->height)) &&
           epd_gfx_egf_stream_read_exact(stream, &record->xoffset, sizeof(record->xoffset)) &&
           epd_gfx_egf_stream_read_exact(stream, &record->yoffset, sizeof(record->yoffset)) &&
           epd_gfx_egf_stream_read_exact(stream, &record->advance, sizeof(record->advance)) &&
           epd_gfx_egf_stream_read_exact(stream, &record->data_offset, sizeof(record->data_offset));
}

static EPD_INLINE bool epd_gfx_egf_seek_size_record(const epd_stream_t* stream, uint32_t index)
{
    int64_t offset = (int64_t)EPD_GFX_EGF_SIZE_TABLE_OFFSET + (int64_t)index * (int64_t)EPD_GFX_EGF_SIZE_RECORD_BYTES;
    return stream->seek(stream->ctx, offset, EPD_SEEK_SET);
}

static EPD_INLINE epd_err_t epd_gfx_egf_read_size_record_at(const epd_stream_t* stream, uint32_t index,
    epd_gfx_egf_size_record_t* record)
{
    if (!epd_gfx_egf_seek_size_record(stream, index)) {
        return EPD_ERR_INVALID_STATE;
    }
    if (!epd_gfx_egf_read_size_record(stream, record)) {
        return EPD_ERR_INVALID_RESPONSE;
    }
    return EPD_OK;
}

static EPD_INLINE bool epd_gfx_egf_seek_glyph_index(const epd_stream_t* stream, uint32_t base_offset, uint32_t index)
{
    int64_t offset = (int64_t)base_offset + (int64_t)index * (int64_t)EPD_GFX_EGF_GLYPH_INDEX_BYTES;
    return stream->seek(stream->ctx, offset, EPD_SEEK_SET);
}

static EPD_INLINE epd_err_t epd_gfx_egf_read_glyph_index_at(const epd_stream_t* stream, uint32_t base_offset,
    uint32_t index, epd_gfx_egf_glyph_index_t* record)
{
    if (!epd_gfx_egf_seek_glyph_index(stream, base_offset, index)) {
        return EPD_ERR_INVALID_STATE;
    }
    if (!epd_gfx_egf_read_glyph_index(stream, record)) {
        return EPD_ERR_INVALID_RESPONSE;
    }
    return EPD_OK;
}

struct epd_gfx_font_impl {
    epd_gfx_egf_header_t header;
    epd_stream_t         stream;

    uint32_t glyph_index_table_offset;
    uint32_t glyph_data_table_offset;
};

#ifdef __cplusplus
}
#endif

#endif // !_EPD_GFX_FONT_IMPL_H_
