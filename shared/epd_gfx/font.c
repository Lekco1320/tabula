/**
 * @file font.c
 * @brief Custom font API and storage.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2025-12-22
 * @license MIT
 */

#include <stdlib.h>
#include <epd_gfx/egf.h>
#include <epd_gfx/font.h>

#include "epd_gfx/font_impl.h"
#include "epd_gfx/glyph_impl.h"

static epd_err_t epd_gfx_font_find_size(const epd_stream_t* stream, uint32_t size_count, uint16_t target_size,
    epd_gfx_egf_size_record_t* out_record)
{
    uint32_t low  = 0U;
    uint32_t high = size_count;
    while (low < high) {
        uint32_t                  mid    = low + (high - low) / 2U;
        epd_gfx_egf_size_record_t record = { 0 };
        epd_err_t                 ret    = epd_gfx_egf_read_size_record_at(stream, mid, &record);
        if (ret != EPD_OK) {
            return ret;
        }
        if (record.size < target_size) {
            low = mid + 1U;
        } else {
            high = mid;
        }
    }

    if (low >= size_count) {
        return EPD_ERR_NOT_FOUND;
    }

    epd_err_t ret = epd_gfx_egf_read_size_record_at(stream, low, out_record);
    if (ret != EPD_OK) {
        return ret;
    }
    if (out_record->size != target_size) {
        return EPD_ERR_NOT_FOUND;
    }

    return EPD_OK;
}

static epd_err_t epd_gfx_font_find_glyph_in_size(const epd_gfx_font_t font,
    const epd_gfx_egf_size_record_t* size_record, uint32_t codepoint, epd_gfx_egf_glyph_index_t* out_index)
{
    if (size_record->glyph_count == 0U) {
        return EPD_ERR_NOT_FOUND;
    }

    const epd_stream_t* stream      = &font->stream;
    uint32_t            base_offset = font->glyph_index_table_offset + size_record->glyph_index_offset;
    uint32_t            low         = 0U;
    uint32_t            high        = size_record->glyph_count;
    epd_err_t           ret         = EPD_OK;
    while (low < high) {
        uint32_t                  mid    = low + (high - low) / 2U;
        epd_gfx_egf_glyph_index_t record = { 0 };
        ret = epd_gfx_egf_read_glyph_index_at(stream, base_offset, mid, &record);
        if (ret != EPD_OK) {
            return ret;
        }
        if (record.codepoint < codepoint) {
            low = mid + 1U;
        } else {
            high = mid;
        }
    }

    if (low >= size_record->glyph_count) {
        return EPD_ERR_NOT_FOUND;
    }

    epd_gfx_egf_glyph_index_t record = { 0 };
    ret = epd_gfx_egf_read_glyph_index_at(stream, base_offset, low, &record);
    if (ret != EPD_OK) {
        return ret;
    }
    if (record.codepoint != codepoint) {
        return EPD_ERR_NOT_FOUND;
    }

    *out_index = record;
    return EPD_OK;
}

static epd_err_t epd_gfx_font_find_glyph(const epd_gfx_font_t font, epd_gfx_glyph_key_t key,
    epd_gfx_egf_size_record_t* out_size, epd_gfx_egf_glyph_index_t* out_index)
{
    if (key.size == 0U) {
        return EPD_ERR_INVALID_ARG;
    }

    const epd_stream_t* stream = &font->stream;

    epd_gfx_egf_size_record_t record = { 0 };

    epd_err_t ret = epd_gfx_font_find_size(stream, font->header.size_count, key.size, &record);
    if (ret != EPD_OK) {
        return ret;
    }

    ret = epd_gfx_font_find_glyph_in_size(font, &record, key.codepoint, out_index);
    if (ret == EPD_OK) {
        *out_size = record;
    }
    return ret;
}

epd_err_t epd_gfx_font_load(const epd_stream_t* stream, epd_gfx_font_t* out_font)
{
    if (!stream || !out_font) {
        return EPD_ERR_INVALID_ARG;
    }

    epd_gfx_font_t font = (epd_gfx_font_t)calloc(1, sizeof(struct epd_gfx_font_impl));
    if (!font) {
        return EPD_ERR_NO_MEM;
    }

    epd_err_t            ret    = EPD_OK;
    epd_gfx_egf_header_t header = { 0 };
    if (!epd_stream_seek(stream, 0, EPD_SEEK_SET)) {
        ret = EPD_ERR_INVALID_STATE;
        goto fail;
    }
    if (!epd_gfx_egf_read_header(stream, &header)) {
        ret = EPD_ERR_INVALID_RESPONSE;
        goto fail;
    }
    if (!epd_gfx_egf_check_magic(&header)) {
        ret = EPD_ERR_INVALID_VERSION;
        goto fail;
    }

    font->header                   = header;
    font->stream                   = *stream;
    font->glyph_index_table_offset = EPD_GFX_EGF_SIZE_TABLE_OFFSET +
        header.size_count * EPD_GFX_EGF_SIZE_RECORD_BYTES;
    font->glyph_data_table_offset  = font->glyph_index_table_offset +
        header.glyph_count * EPD_GFX_EGF_GLYPH_INDEX_BYTES;

    *out_font = font;
    return EPD_OK;

fail:
    free(font);
    return ret;
}

epd_err_t epd_gfx_font_destroy(epd_gfx_font_t font)
{
    if (font) {
        free(font);
    }

    return EPD_OK;
}

epd_err_t epd_gfx_font_get_size_info(const epd_gfx_font_t font,
    uint16_t size, epd_gfx_font_size_info_t* out_info)
{
    if (!font || size == 0U || !out_info) {
        return EPD_ERR_INVALID_ARG;
    }

    const epd_stream_t* stream = &font->stream;
    epd_gfx_egf_size_record_t record = { 0 };
    epd_err_t                 ret    = epd_gfx_font_find_size(stream, font->header.size_count, size, &record);
    if (ret != EPD_OK) {
        return ret;
    }

    out_info->size        = record.size;
    out_info->ascent      = record.ascent;
    out_info->descent     = record.descent;
    out_info->line_height = record.line_height;
    out_info->glyph_count = record.glyph_count;
    return EPD_OK;
}

bool epd_gfx_font_contains_size(const epd_gfx_font_t font, uint16_t size)
{
    if (!font || size == 0U) {
        return false;
    }

    const epd_stream_t* stream = &font->stream;
    epd_gfx_egf_size_record_t record = { 0 };
    return epd_gfx_font_find_size(stream, font->header.size_count, size, &record) == EPD_OK;
}

epd_err_t epd_gfx_font_load_glyph(const epd_gfx_font_t font, epd_gfx_glyph_key_t key,
    epd_gfx_glyph_t* out_glyph)
{
    if (!font || !out_glyph) {
        return EPD_ERR_INVALID_ARG;
    }

    const epd_stream_t* stream = &font->stream;
    epd_gfx_egf_size_record_t size_record = { 0 };
    epd_gfx_egf_glyph_index_t glyph_index = { 0 };
    uint8_t*                  data        = NULL;
    epd_gfx_glyph_t           glyph       = NULL;
    epd_err_t                 ret         = epd_gfx_font_find_glyph(font, key, &size_record, &glyph_index);
    if (ret != EPD_OK) {
        goto clean;
    }

    size_t size = epd_gfx_glyph_data_bytes(glyph_index.width, glyph_index.height);
    if (size > 0U) {
        data = (uint8_t*)malloc(size);
        if (!data) {
            ret = EPD_ERR_NO_MEM;
            goto clean;
        }
        uint32_t data_offset = font->glyph_data_table_offset + size_record.glyph_data_offset + glyph_index.data_offset;
        if (!epd_stream_seek(stream, (int64_t)data_offset, EPD_SEEK_SET)) {
            ret = EPD_ERR_INVALID_STATE;
            goto clean;
        }
        if (!epd_stream_read_exact(stream, data, size)) {
            ret = EPD_ERR_INVALID_RESPONSE;
            goto clean;
        }
    }

    glyph = (epd_gfx_glyph_t)calloc(1, sizeof(struct epd_gfx_glyph_impl));
    if (!glyph) {
        ret = EPD_ERR_NO_MEM;
        goto clean;
    }

    glyph->width       = glyph_index.width;
    glyph->height      = glyph_index.height;
    glyph->xoffset     = glyph_index.xoffset;
    glyph->yoffset     = glyph_index.yoffset;
    glyph->advance     = glyph_index.advance;
    glyph->data        = data;

    data       = NULL;
    *out_glyph = glyph;
    glyph      = NULL;
    ret        = EPD_OK;

clean:
    if (glyph) {
        free(glyph);
    }
    if (data) {
        free(data);
    }
    return ret;
}

bool epd_gfx_font_contains_glyph(const epd_gfx_font_t font, epd_gfx_glyph_key_t key)
{
    if (!font) {
        return false;
    }

    epd_gfx_egf_size_record_t size_record = { 0 };
    epd_gfx_egf_glyph_index_t glyph_index = { 0 };
    return epd_gfx_font_find_glyph(font, key, &size_record, &glyph_index) == EPD_OK;
}
