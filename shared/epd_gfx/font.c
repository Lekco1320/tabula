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

static epd_err_t epd_gfx_font_lower_bound_size(const epd_stream_t* stream, uint32_t size_count, uint16_t target_size,
    uint32_t* out_index, epd_gfx_egf_size_record_t* out_record, bool* out_exact)
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

    *out_index = low;
    if (low >= size_count) {
        *out_exact = false;
        return EPD_OK;
    }

    epd_err_t ret = epd_gfx_egf_read_size_record_at(stream, low, out_record);
    if (ret != EPD_OK) {
        return ret;
    }
    *out_exact = (out_record->size == target_size);
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

static epd_err_t epd_gfx_font_find_glyph_in_size_range(const epd_gfx_font_t font, uint32_t size_count, int32_t start,
    int32_t step, uint32_t codepoint, epd_gfx_egf_size_record_t* out_size, epd_gfx_egf_glyph_index_t* out_index)
{
    const epd_stream_t* stream = &font->stream;
    for (int32_t i = start; i >= 0 && i < (int32_t)size_count; i += step) {
        epd_gfx_egf_size_record_t record = { 0 };
        epd_err_t                 ret    = epd_gfx_egf_read_size_record_at(stream, (uint32_t)i, &record);
        if (ret != EPD_OK) {
            return ret;
        }
        ret = epd_gfx_font_find_glyph_in_size(font, &record, codepoint, out_index);
        if (ret == EPD_OK) {
            *out_size = record;
            return EPD_OK;
        }
        if (ret != EPD_ERR_NOT_FOUND) {
            return ret;
        }
    }

    return EPD_ERR_NOT_FOUND;
}

static epd_err_t epd_gfx_font_find_glyph(const epd_gfx_font_t font, epd_gfx_glyph_seek_config_t config,
    epd_gfx_egf_size_record_t* out_size, epd_gfx_egf_glyph_index_t* out_index, bool* out_fallback_used)
{
    if (config.size == 0U || config.fallback > EPD_GFX_GLYPH_FALLBACK_LARGER) {
        return EPD_ERR_INVALID_ARG;
    }

    const epd_stream_t* stream = &font->stream;
    if (out_fallback_used) {
        *out_fallback_used = false;
    }

    uint32_t                  index  = 0U;
    bool                      exact  = false;
    epd_gfx_egf_size_record_t record = { 0 };

    epd_err_t ret = epd_gfx_font_lower_bound_size(stream, font->header.size_count, config.size, &index, &record,
        &exact);
    if (ret != EPD_OK) {
        return ret;
    }

    if (exact) {
        ret = epd_gfx_font_find_glyph_in_size(font, &record, config.codepoint, out_index);
        if (ret == EPD_OK) {
            *out_size = record;
            return EPD_OK;
        }
    } else {
        ret = EPD_ERR_NOT_FOUND;
    }

    if (ret != EPD_ERR_NOT_FOUND || config.fallback == EPD_GFX_GLYPH_FALLBACK_NONE) {
        return ret;
    }

    int32_t start = -1;
    int32_t step  = 0;
    if (config.fallback == EPD_GFX_GLYPH_FALLBACK_SMALLER) {
        start = (int32_t)index - 1;
        step  = -1;
    } else if (config.fallback == EPD_GFX_GLYPH_FALLBACK_LARGER) {
        start = exact ? (int32_t)index + 1 : (int32_t)index;
        step  = 1;
    } else {
        return EPD_ERR_INVALID_ARG;
    }

    if (start < 0 || start >= (int32_t)font->header.size_count) {
        return EPD_ERR_NOT_FOUND;
    }

    ret = epd_gfx_font_find_glyph_in_size_range(font, font->header.size_count, start, step, config.codepoint,
        out_size, out_index);
    if (ret == EPD_OK && out_fallback_used) {
        *out_fallback_used = true;
    }
    return ret;
}

epd_err_t epd_gfx_font_load(const epd_stream_t* stream, epd_gfx_font_t* out_font)
{
    if (!stream || !out_font || !stream->read || !stream->seek) {
        return EPD_ERR_INVALID_ARG;
    }

    epd_gfx_font_t font = (epd_gfx_font_t)calloc(1, sizeof(struct epd_gfx_font_impl));
    if (!font) {
        return EPD_ERR_NO_MEM;
    }

    epd_err_t            ret    = EPD_OK;
    epd_gfx_egf_header_t header = { 0 };
    if (!stream->seek(stream->ctx, 0, EPD_SEEK_SET)) {
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

epd_err_t epd_gfx_font_get_glyph(const epd_gfx_font_t font, epd_gfx_glyph_seek_config_t config,
    epd_gfx_glyph_t* out_glyph)
{
    if (!font || !out_glyph) {
        return EPD_ERR_INVALID_ARG;
    }

    const epd_stream_t* stream = &font->stream;
    if (!stream->read || !stream->seek) {
        return EPD_ERR_NOT_SUPPORTED;
    }

    if (font->header.size_count == 0U) {
        return EPD_ERR_NOT_FOUND;
    }

    epd_gfx_egf_size_record_t size_record   = { 0 };
    epd_gfx_egf_glyph_index_t glyph_index   = { 0 };
    bool                      fallback_used = false;
    uint8_t*                  data          = NULL;
    epd_gfx_glyph_t           glyph         = NULL;
    epd_err_t                 ret           = epd_gfx_font_find_glyph(font, config, &size_record, &glyph_index,
        &fallback_used);
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
        if (!stream->seek(stream->ctx, (int64_t)data_offset, EPD_SEEK_SET)) {
            ret = EPD_ERR_INVALID_STATE;
            goto clean;
        }
        if (stream->read(stream->ctx, data, size) != size) {
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
    glyph->ascent      = size_record.ascent;
    glyph->line_height = size_record.line_height;

    data       = NULL;
    *out_glyph = glyph;
    glyph      = NULL;
    ret        = fallback_used ? EPD_FALLBACK : EPD_OK;

clean:
    if (glyph) {
        free(glyph);
    }
    if (data) {
        free(data);
    }
    return ret;
}

bool epd_gfx_font_contains_glyph(const epd_gfx_font_t font, epd_gfx_glyph_seek_config_t config)
{
    if (!font) {
        return false;
    }

    const epd_stream_t* stream = &font->stream;
    if (!stream->read || !stream->seek || font->header.size_count == 0U) {
        return false;
    }

    epd_gfx_egf_size_record_t size_record = { 0 };
    epd_gfx_egf_glyph_index_t glyph_index = { 0 };
    return epd_gfx_font_find_glyph(font, config, &size_record, &glyph_index, NULL) == EPD_OK;
}
