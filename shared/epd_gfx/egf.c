/**
 * @file egf.c
 * @brief EGF1 font file format helper implementation.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2026-05-13
 * @license MIT
 */

#include <stdint.h>
#include <epd_gfx/egf.h>

bool epd_gfx_egf_stream_read_exact(const epd_stream_t* stream, void* dst, size_t size)
{
    if (!stream || !stream->read || !dst) {
        return false;
    }

    return stream->read(stream->ctx, dst, size) == size;
}

bool epd_gfx_egf_check_magic(const epd_gfx_egf_header_t* header)
{
    if (!header) {
        return false;
    }

    return header->magic[0] == EPD_GFX_EGF_MAGIC[0] &&
           header->magic[1] == EPD_GFX_EGF_MAGIC[1] &&
           header->magic[2] == EPD_GFX_EGF_MAGIC[2] &&
           header->magic[3] == EPD_GFX_EGF_MAGIC[3];
}

bool epd_gfx_egf_read_header(const epd_stream_t* stream, epd_gfx_egf_header_t* header)
{
    if (!header) {
        return false;
    }

    return epd_gfx_egf_stream_read_exact(stream, header->magic, sizeof(header->magic)) &&
           epd_gfx_egf_stream_read_exact(stream, &header->size_count, sizeof(header->size_count)) &&
           epd_gfx_egf_stream_read_exact(stream, &header->glyph_count, sizeof(header->glyph_count)) &&
           epd_gfx_egf_stream_read_exact(stream, &header->data_count, sizeof(header->data_count));
}

bool epd_gfx_egf_read_size_record(const epd_stream_t* stream, epd_gfx_egf_size_record_t* record)
{
    if (!record) {
        return false;
    }

    return epd_gfx_egf_stream_read_exact(stream, &record->size, sizeof(record->size)) &&
           epd_gfx_egf_stream_read_exact(stream, &record->ascent, sizeof(record->ascent)) &&
           epd_gfx_egf_stream_read_exact(stream, &record->descent, sizeof(record->descent)) &&
           epd_gfx_egf_stream_read_exact(stream, &record->line_height, sizeof(record->line_height)) &&
           epd_gfx_egf_stream_read_exact(stream, &record->glyph_count, sizeof(record->glyph_count)) &&
           epd_gfx_egf_stream_read_exact(stream, &record->glyph_index_offset, sizeof(record->glyph_index_offset)) &&
           epd_gfx_egf_stream_read_exact(stream, &record->glyph_data_offset, sizeof(record->glyph_data_offset));
}

bool epd_gfx_egf_read_glyph_index(const epd_stream_t* stream, epd_gfx_egf_glyph_index_t* record)
{
    if (!record) {
        return false;
    }

    return epd_gfx_egf_stream_read_exact(stream, &record->codepoint, sizeof(record->codepoint)) &&
           epd_gfx_egf_stream_read_exact(stream, &record->width, sizeof(record->width)) &&
           epd_gfx_egf_stream_read_exact(stream, &record->height, sizeof(record->height)) &&
           epd_gfx_egf_stream_read_exact(stream, &record->xoffset, sizeof(record->xoffset)) &&
           epd_gfx_egf_stream_read_exact(stream, &record->yoffset, sizeof(record->yoffset)) &&
           epd_gfx_egf_stream_read_exact(stream, &record->advance, sizeof(record->advance)) &&
           epd_gfx_egf_stream_read_exact(stream, &record->data_offset, sizeof(record->data_offset));
}

bool epd_gfx_egf_seek_size_record(const epd_stream_t* stream, uint32_t index)
{
    if (!stream || !stream->seek) {
        return false;
    }

    int64_t offset = (int64_t)EPD_GFX_EGF_SIZE_TABLE_OFFSET +
        (int64_t)index * (int64_t)EPD_GFX_EGF_SIZE_RECORD_BYTES;
    return stream->seek(stream->ctx, offset, EPD_SEEK_SET);
}

epd_err_t epd_gfx_egf_read_size_record_at(const epd_stream_t* stream, uint32_t index,
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

bool epd_gfx_egf_seek_glyph_index(const epd_stream_t* stream, uint32_t base_offset, uint32_t index)
{
    if (!stream || !stream->seek) {
        return false;
    }

    int64_t offset = (int64_t)base_offset + (int64_t)index * (int64_t)EPD_GFX_EGF_GLYPH_INDEX_BYTES;
    return stream->seek(stream->ctx, offset, EPD_SEEK_SET);
}

epd_err_t epd_gfx_egf_read_glyph_index_at(const epd_stream_t* stream, uint32_t base_offset,
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
