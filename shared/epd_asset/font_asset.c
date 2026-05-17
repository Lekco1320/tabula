/**
 * @file font_asset.c
 * @brief Editable font asset API for offline EGF font generation.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2026-04-28
 * @license MIT
 */

#include <stdlib.h>
#include <string.h>
#include <epd_asset/font_asset.h>
#include <epd_gfx/egf.h>

#include "epd_asset/font_asset_impl.h"

typedef struct {
    uint32_t size_count;
    uint32_t glyph_count;
    uint32_t data_count;
} epd_asset_font_asset_stats_t;

static epd_err_t epd_asset_font_asset_copy_glyph_data(const epd_gfx_glyph_t glyph, uint8_t** out_data)
{
    uint32_t data_bytes = epd_gfx_glyph_data_bytes(epd_gfx_glyph_get_width(glyph),
        epd_gfx_glyph_get_height(glyph));
    uint8_t* data       = NULL;

    if (data_bytes > 0U) {
        const uint8_t* glyph_data = epd_gfx_glyph_get_data(glyph);
        if (!glyph_data) {
            return EPD_ERR_INVALID_ARG;
        }

        data = (uint8_t*)malloc(data_bytes);
        if (!data) {
            return EPD_ERR_NO_MEM;
        }
        memcpy(data, glyph_data, data_bytes);
    }

    *out_data = data;
    return EPD_OK;
}

static void epd_asset_font_asset_glyph_destroy(epd_asset_font_asset_glyph_t* glyph)
{
    if (!glyph) {
        return;
    }

    if (glyph->data) {
        free(glyph->data);
        glyph->data = NULL;
    }
}

static void epd_asset_font_asset_size_destroy(epd_asset_font_asset_size_t* size)
{
    if (!size) {
        return;
    }

    epd_asset_font_asset_glyph_node_t cur = size->glyph_list;
    while (cur) {
        epd_asset_font_asset_glyph_node_t next = cur->next;
        epd_asset_font_asset_glyph_destroy(&cur->value);
        free(cur);
        cur = next;
    }
    size->glyph_list = NULL;
}

static epd_asset_font_asset_size_node_t epd_asset_font_asset_find_size(const epd_asset_font_asset_t asset, uint16_t size)
{
    epd_asset_font_asset_size_node_t cur = asset->size_list;
    while (cur && cur->value.size < size) {
        cur = cur->next;
    }

    return (cur && cur->value.size == size) ? cur : NULL;
}

static epd_asset_font_asset_glyph_node_t epd_asset_font_asset_find_glyph(
    const epd_asset_font_asset_size_t* size, uint32_t codepoint)
{
    epd_asset_font_asset_glyph_node_t cur = size->glyph_list;
    while (cur && cur->value.codepoint < codepoint) {
        cur = cur->next;
    }

    return (cur && cur->value.codepoint == codepoint) ? cur : NULL;
}

static epd_err_t epd_asset_font_asset_put_glyph(epd_asset_font_asset_size_node_t size_node,
    epd_asset_font_asset_glyph_t glyph)
{
    epd_asset_font_asset_glyph_node_t pre = NULL;
    epd_asset_font_asset_glyph_node_t cur = size_node->value.glyph_list;
    while (cur && cur->value.codepoint < glyph.codepoint) {
        pre = cur;
        cur = cur->next;
    }

    if (cur && cur->value.codepoint == glyph.codepoint) {
        epd_asset_font_asset_glyph_destroy(&cur->value);
        cur->value = glyph;
        return EPD_OK;
    }

    epd_asset_font_asset_glyph_node_t glyph_node = (epd_asset_font_asset_glyph_node_t)calloc(1,
        sizeof(struct epd_asset_font_asset_glyph_node));
    if (!glyph_node) {
        return EPD_ERR_NO_MEM;
    }

    glyph_node->value = glyph;
    glyph_node->next  = cur;

    if (pre) {
        pre->next = glyph_node;
    } else {
        size_node->value.glyph_list = glyph_node;
    }

    return EPD_OK;
}

static epd_asset_font_asset_stats_t epd_asset_font_asset_size_stats(const epd_asset_font_asset_size_t* size)
{
    epd_asset_font_asset_stats_t      stats = { 0 };
    epd_asset_font_asset_glyph_node_t cur   = size->glyph_list;
    while (cur) {
        stats.glyph_count += 1U;
        stats.data_count  += epd_gfx_glyph_data_bytes(cur->value.width, cur->value.height);
        cur = cur->next;
    }

    return stats;
}

static epd_asset_font_asset_stats_t epd_asset_font_asset_stats(const epd_asset_font_asset_t asset)
{
    epd_asset_font_asset_stats_t     stats = { 0 };
    epd_asset_font_asset_size_node_t cur   = asset->size_list;
    while (cur) {
        epd_asset_font_asset_stats_t size_stats = epd_asset_font_asset_size_stats(&cur->value);
        stats.size_count  += 1U;
        stats.glyph_count += size_stats.glyph_count;
        stats.data_count  += size_stats.data_count;
        cur                = cur->next;
    }

    return stats;
}

static epd_err_t epd_asset_font_asset_write_exact(epd_stream_t* stream, const void* data, size_t size)
{
    return epd_stream_write_exact(stream, data, size) ? EPD_OK : EPD_ERR_INVALID_STATE;
}

static epd_err_t epd_asset_font_asset_write_header(epd_stream_t* stream, uint32_t size_count,
    uint32_t glyph_count, uint32_t data_count)
{
    epd_err_t ret = EPD_OK;
    EPD_CHECK_RET(epd_asset_font_asset_write_exact(stream, EPD_GFX_EGF_MAGIC, EPD_GFX_EGF_MAGIC_BYTES));
    EPD_CHECK_RET(epd_asset_font_asset_write_exact(stream, &size_count, sizeof(size_count)));
    EPD_CHECK_RET(epd_asset_font_asset_write_exact(stream, &glyph_count, sizeof(glyph_count)));

    return epd_asset_font_asset_write_exact(stream, &data_count, sizeof(data_count));
}

static epd_err_t epd_asset_font_asset_write_size_record(epd_stream_t* stream,
    const epd_asset_font_asset_size_t* size, uint32_t glyph_count, uint32_t glyph_index_offset,
    uint32_t glyph_data_offset)
{
    epd_err_t ret = EPD_OK;
    EPD_CHECK_RET(epd_asset_font_asset_write_exact(stream, &size->size, sizeof(size->size)));
    EPD_CHECK_RET(epd_asset_font_asset_write_exact(stream, &size->ascent, sizeof(size->ascent)));
    EPD_CHECK_RET(epd_asset_font_asset_write_exact(stream, &size->descent, sizeof(size->descent)));
    EPD_CHECK_RET(epd_asset_font_asset_write_exact(stream, &size->line_height, sizeof(size->line_height)));
    EPD_CHECK_RET(epd_asset_font_asset_write_exact(stream, &glyph_count, sizeof(glyph_count)));
    EPD_CHECK_RET(epd_asset_font_asset_write_exact(stream, &glyph_index_offset, sizeof(glyph_index_offset)));

    return epd_asset_font_asset_write_exact(stream, &glyph_data_offset, sizeof(glyph_data_offset));
}

static epd_err_t epd_asset_font_asset_write_glyph_index(epd_stream_t* stream,
    const epd_asset_font_asset_glyph_t* glyph, uint32_t data_offset)
{
    epd_err_t ret = EPD_OK;
    EPD_CHECK_RET(epd_asset_font_asset_write_exact(stream, &glyph->codepoint, sizeof(glyph->codepoint)));
    EPD_CHECK_RET(epd_asset_font_asset_write_exact(stream, &glyph->width, sizeof(glyph->width)));
    EPD_CHECK_RET(epd_asset_font_asset_write_exact(stream, &glyph->height, sizeof(glyph->height)));
    EPD_CHECK_RET(epd_asset_font_asset_write_exact(stream, &glyph->xoffset, sizeof(glyph->xoffset)));
    EPD_CHECK_RET(epd_asset_font_asset_write_exact(stream, &glyph->yoffset, sizeof(glyph->yoffset)));
    EPD_CHECK_RET(epd_asset_font_asset_write_exact(stream, &glyph->advance, sizeof(glyph->advance)));

    return epd_asset_font_asset_write_exact(stream, &data_offset, sizeof(data_offset));
}

static epd_err_t epd_asset_font_asset_write_size_table(const epd_asset_font_asset_t asset, epd_stream_t* stream)
{
    uint32_t  glyph_index_offset = 0U;
    uint32_t  glyph_data_offset  = 0U;
    epd_err_t ret                = EPD_OK;

    epd_asset_font_asset_size_node_t cur = asset->size_list;
    while (cur) {
        epd_asset_font_asset_stats_t stats = epd_asset_font_asset_size_stats(&cur->value);
        EPD_CHECK_RET(epd_asset_font_asset_write_size_record(stream, &cur->value, stats.glyph_count, glyph_index_offset,
            glyph_data_offset));

        glyph_index_offset += stats.glyph_count * EPD_GFX_EGF_GLYPH_INDEX_BYTES;
        glyph_data_offset  += stats.data_count;
        cur                 = cur->next;
    }

    return EPD_OK;
}

static epd_err_t epd_asset_font_asset_write_glyph_index_table(const epd_asset_font_asset_t asset, epd_stream_t* stream)
{
    epd_err_t                        ret      = EPD_OK;
    epd_asset_font_asset_size_node_t size_cur = asset->size_list;
    while (size_cur) {
        uint32_t                         data_offset = 0U;
        epd_asset_font_asset_glyph_node_t glyph_cur   = size_cur->value.glyph_list;
        while (glyph_cur) {
            EPD_CHECK_RET(epd_asset_font_asset_write_glyph_index(stream, &glyph_cur->value, data_offset));

            data_offset += epd_gfx_glyph_data_bytes(glyph_cur->value.width, glyph_cur->value.height);
            glyph_cur    = glyph_cur->next;
        }

        size_cur = size_cur->next;
    }

    return EPD_OK;
}

static epd_err_t epd_asset_font_asset_write_glyph_data_table(const epd_asset_font_asset_t asset, epd_stream_t* stream)
{
    epd_err_t                        ret      = EPD_OK;
    epd_asset_font_asset_size_node_t size_cur = asset->size_list;
    while (size_cur) {
        epd_asset_font_asset_glyph_node_t glyph_cur = size_cur->value.glyph_list;
        while (glyph_cur) {
            uint32_t data_bytes = epd_gfx_glyph_data_bytes(glyph_cur->value.width, glyph_cur->value.height);
            if (data_bytes > 0U) {
                if (!glyph_cur->value.data) {
                    return EPD_ERR_INVALID_STATE;
                }

                EPD_CHECK_RET(epd_asset_font_asset_write_exact(stream, glyph_cur->value.data, data_bytes));
            }
            glyph_cur = glyph_cur->next;
        }
        size_cur = size_cur->next;
    }

    return EPD_OK;
}

static epd_err_t epd_asset_font_asset_load_glyph_data(const epd_stream_t* stream, uint32_t data_offset,
    uint32_t data_bytes, uint8_t** out_data)
{
    uint8_t* data = NULL;
    if (data_bytes > 0U) {
        data = (uint8_t*)malloc(data_bytes);
        if (!data) {
            return EPD_ERR_NO_MEM;
        }

        if (!epd_stream_seek(stream, (int64_t)data_offset, EPD_SEEK_SET)) {
            free(data);
            return EPD_ERR_INVALID_STATE;
        }
        if (!epd_stream_read_exact(stream, data, data_bytes)) {
            free(data);
            return EPD_ERR_INVALID_RESPONSE;
        }
    }

    *out_data = data;
    return EPD_OK;
}

epd_err_t epd_asset_font_asset_create(epd_asset_font_asset_t* out_asset)
{
    if (!out_asset) {
        return EPD_ERR_INVALID_ARG;
    }

    epd_asset_font_asset_t asset = (epd_asset_font_asset_t)calloc(1, sizeof(struct epd_asset_font_asset_impl));
    if (!asset) {
        return EPD_ERR_NO_MEM;
    }

    asset->size_list = NULL;
    *out_asset       = asset;
    return EPD_OK;
}

epd_err_t epd_asset_font_asset_load_egf(const epd_stream_t* stream, epd_asset_font_asset_t* out_asset)
{
    if (!stream || !out_asset) {
        return EPD_ERR_INVALID_ARG;
    }
    *out_asset = NULL;

    epd_err_t            ret    = EPD_OK;
    epd_gfx_egf_header_t header = { 0 };
    if (!epd_stream_seek(stream, 0, EPD_SEEK_SET)) {
        return EPD_ERR_INVALID_STATE;
    }
    if (!epd_gfx_egf_read_header(stream, &header)) {
        return EPD_ERR_INVALID_RESPONSE;
    }
    if (!epd_gfx_egf_check_magic(&header)) {
        return EPD_ERR_INVALID_VERSION;
    }

    epd_asset_font_asset_t asset = (epd_asset_font_asset_t)calloc(1, sizeof(struct epd_asset_font_asset_impl));
    if (!asset) {
        return EPD_ERR_NO_MEM;
    }

    asset->size_list = NULL;

    uint32_t glyph_index_table_offset = EPD_GFX_EGF_SIZE_TABLE_OFFSET +
        header.size_count * EPD_GFX_EGF_SIZE_RECORD_BYTES;
    uint32_t glyph_data_table_offset  = glyph_index_table_offset +
        header.glyph_count * EPD_GFX_EGF_GLYPH_INDEX_BYTES;

    for (uint32_t i = 0U; i < header.size_count; ++i) {
        epd_gfx_egf_size_record_t size_record = { 0 };
        EPD_CHECK_GOTO(epd_gfx_egf_read_size_record_at(stream, i, &size_record), fail);
        if (size_record.size == 0U) {
            ret = EPD_ERR_INVALID_RESPONSE;
            goto fail;
        }

        epd_asset_font_asset_size_info_t size_info;
        size_info.size        = size_record.size;
        size_info.ascent      = size_record.ascent;
        size_info.descent     = size_record.descent;
        size_info.line_height = size_record.line_height;
        EPD_CHECK_GOTO(epd_asset_font_asset_set_size_info(asset, &size_info), fail);

        epd_asset_font_asset_size_node_t size_node = epd_asset_font_asset_find_size(asset, size_record.size);
        if (!size_node) {
            ret = EPD_ERR_INVALID_STATE;
            goto fail;
        }

        uint32_t glyph_index_base = glyph_index_table_offset + size_record.glyph_index_offset;
        for (uint32_t j = 0U; j < size_record.glyph_count; ++j) {
            epd_gfx_egf_glyph_index_t glyph_index = { 0 };
            EPD_CHECK_GOTO(epd_gfx_egf_read_glyph_index_at(stream, glyph_index_base, j, &glyph_index), fail);

            uint32_t data_bytes  = epd_gfx_glyph_data_bytes(glyph_index.width, glyph_index.height);
            uint32_t data_offset = glyph_data_table_offset + size_record.glyph_data_offset + glyph_index.data_offset;
            uint8_t* data        = NULL;
            EPD_CHECK_GOTO(epd_asset_font_asset_load_glyph_data(stream, data_offset, data_bytes, &data), fail);

            epd_asset_font_asset_glyph_t glyph;
            glyph.codepoint = glyph_index.codepoint;
            glyph.width     = glyph_index.width;
            glyph.height    = glyph_index.height;
            glyph.xoffset   = glyph_index.xoffset;
            glyph.yoffset   = glyph_index.yoffset;
            glyph.advance   = glyph_index.advance;
            glyph.data      = data;
            ret = epd_asset_font_asset_put_glyph(size_node, glyph);
            if (ret != EPD_OK) {
                free(data);
                goto fail;
            }
        }
    }

    *out_asset = asset;
    return EPD_OK;

fail:
    epd_asset_font_asset_destroy(asset);
    return ret;
}

epd_err_t epd_asset_font_asset_destroy(epd_asset_font_asset_t asset)
{
    if (asset) {
        epd_asset_font_asset_size_node_t cur = asset->size_list;
        while (cur) {
            epd_asset_font_asset_size_node_t next = cur->next;
            epd_asset_font_asset_size_destroy(&cur->value);
            free(cur);
            cur = next;
        }
        free(asset);
    }

    return EPD_OK;
}

epd_err_t epd_asset_font_asset_get_sizes(const epd_asset_font_asset_t asset, uint16_t* sizes,
    uint32_t* count)
{
    if (!asset || !count) {
        return EPD_ERR_INVALID_ARG;
    }

    if (sizes == NULL) {
        uint32_t                       size_count = 0U;
        epd_asset_font_asset_size_node_t size_node  = asset->size_list;
        while (size_node) {
            ++size_count;
            size_node = size_node->next;
        }
        *count = size_count;
        return EPD_OK;
    }

    epd_asset_font_asset_size_node_t cur     = asset->size_list;
    uint32_t                       written = 0U;
    for (; cur && written < *count; ++written, (cur = cur->next)) {
        sizes[written] = cur->value.size;
    }
    *count = written;
    return EPD_OK;
}

epd_err_t epd_asset_font_asset_get_codepoints(const epd_asset_font_asset_t asset, uint16_t size,
    uint32_t* codepoints, uint32_t* count)
{
    if (!asset || size == 0U || !count) {
        return EPD_ERR_INVALID_ARG;
    }

    epd_asset_font_asset_size_node_t size_node = epd_asset_font_asset_find_size(asset, size);
    if (!size_node) {
        return EPD_ERR_NOT_FOUND;
    }

    if (codepoints == NULL) {
        uint32_t                        glyph_count = 0U;
        epd_asset_font_asset_glyph_node_t glyph_node  = size_node->value.glyph_list;
        while (glyph_node) {
            ++glyph_count;
            glyph_node = glyph_node->next;
        }
        *count = glyph_count;
        return EPD_OK;
    }

    epd_asset_font_asset_glyph_node_t cur     = size_node->value.glyph_list;
    uint32_t                        written = 0U;
    for (; cur && written < *count; ++written, (cur = cur->next)) {
        codepoints[written] = cur->value.codepoint;
    }
    *count = written;
    return EPD_OK;
}

epd_err_t epd_asset_font_asset_set_size_info(epd_asset_font_asset_t asset,
    const epd_asset_font_asset_size_info_t* info)
{
    if (!asset || !info || info->size == 0U) {
        return EPD_ERR_INVALID_ARG;
    }

    epd_asset_font_asset_size_node_t pre = NULL;
    epd_asset_font_asset_size_node_t cur = asset->size_list;
    while (cur && cur->value.size < info->size) {
        pre = cur;
        cur = cur->next;
    }

    if (cur && cur->value.size == info->size) {
        cur->value.ascent      = info->ascent;
        cur->value.descent     = info->descent;
        cur->value.line_height = info->line_height;
        return EPD_OK;
    }

    epd_asset_font_asset_size_node_t node = (epd_asset_font_asset_size_node_t)calloc(1,
        sizeof(struct epd_asset_font_asset_size_node));
    if (!node) {
        return EPD_ERR_NO_MEM;
    }

    node->value.size        = info->size;
    node->value.ascent      = info->ascent;
    node->value.descent     = info->descent;
    node->value.line_height = info->line_height;
    node->value.glyph_list  = NULL;
    node->next              = cur;

    if (pre) {
        pre->next = node;
    } else {
        asset->size_list = node;
    }

    return EPD_OK;
}

epd_err_t epd_asset_font_asset_get_size_info(const epd_asset_font_asset_t asset,
    uint16_t size, epd_asset_font_asset_size_info_t* out_info)
{
    if (!asset || size == 0U || !out_info) {
        return EPD_ERR_INVALID_ARG;
    }

    epd_asset_font_asset_size_node_t size_node = epd_asset_font_asset_find_size(asset, size);
    if (!size_node) {
        return EPD_ERR_NOT_FOUND;
    }

    out_info->size        = size_node->value.size;
    out_info->ascent      = size_node->value.ascent;
    out_info->descent     = size_node->value.descent;
    out_info->line_height = size_node->value.line_height;
    return EPD_OK;
}

epd_err_t epd_asset_font_asset_remove_size(epd_asset_font_asset_t asset, uint16_t size)
{
    if (!asset || size == 0U) {
        return EPD_ERR_INVALID_ARG;
    }

    epd_asset_font_asset_size_node_t pre = NULL;
    epd_asset_font_asset_size_node_t cur = asset->size_list;
    while (cur && cur->value.size < size) {
        pre = cur;
        cur = cur->next;
    }
    if (!cur || cur->value.size != size) {
        return EPD_OK;
    }

    epd_asset_font_asset_size_node_t next = cur->next;
    if (pre) {
        pre->next = next;
    } else {
        asset->size_list = next;
    }
    epd_asset_font_asset_size_destroy(&cur->value);
    free(cur);
    return EPD_OK;
}

epd_err_t epd_asset_font_asset_copy_glyph(const epd_asset_font_asset_t asset,
    epd_asset_font_asset_glyph_key_t key, epd_gfx_glyph_t* out_glyph)
{
    if (!asset || !out_glyph || key.size == 0U) {
        return EPD_ERR_INVALID_ARG;
    }
    *out_glyph = NULL;

    epd_asset_font_asset_size_node_t size_node = epd_asset_font_asset_find_size(asset, key.size);
    if (!size_node) {
        return EPD_ERR_NOT_FOUND;
    }

    epd_asset_font_asset_glyph_node_t glyph_node =
        epd_asset_font_asset_find_glyph(&size_node->value, key.codepoint);
    if (!glyph_node) {
        return EPD_ERR_NOT_FOUND;
    }

    epd_gfx_glyph_config_t config;
    config.width   = glyph_node->value.width;
    config.height  = glyph_node->value.height;
    config.xoffset = glyph_node->value.xoffset;
    config.yoffset = glyph_node->value.yoffset;
    config.advance = glyph_node->value.advance;
    config.data    = glyph_node->value.data;

    return epd_gfx_glyph_create(&config, out_glyph);
}

epd_err_t epd_asset_font_asset_add_glyph(epd_asset_font_asset_t asset,
    epd_asset_font_asset_glyph_key_t key, const epd_gfx_glyph_t glyph)
{
    if (!asset || !glyph || key.size == 0U) {
        return EPD_ERR_INVALID_ARG;
    }

    epd_asset_font_asset_size_node_t node = NULL;
    epd_err_t                      ret  = EPD_OK;
    uint8_t*                       data = NULL;

    node = epd_asset_font_asset_find_size(asset, key.size);
    if (!node) {
        return EPD_ERR_NOT_ALLOWED;
    }

    EPD_CHECK_RET(epd_asset_font_asset_copy_glyph_data(glyph, &data));

    epd_asset_font_asset_glyph_t value;
    value.codepoint = key.codepoint;
    value.width     = epd_gfx_glyph_get_width(glyph);
    value.height    = epd_gfx_glyph_get_height(glyph);
    value.xoffset   = epd_gfx_glyph_get_xoffset(glyph);
    value.yoffset   = epd_gfx_glyph_get_yoffset(glyph);
    value.advance   = epd_gfx_glyph_get_advance(glyph);
    value.data      = data;
    ret = epd_asset_font_asset_put_glyph(node, value);
    if (ret != EPD_OK) {
        free(data);
        return ret;
    }

    return EPD_OK;
}

epd_err_t epd_asset_font_asset_remove_glyph(epd_asset_font_asset_t asset,
    epd_asset_font_asset_glyph_key_t key)
{
    if (!asset || key.size == 0U) {
        return EPD_ERR_INVALID_ARG;
    }

    epd_asset_font_asset_size_node_t size_node = epd_asset_font_asset_find_size(asset, key.size);
    if (!size_node) {
        return EPD_OK;
    }

    epd_asset_font_asset_glyph_node_t pre = NULL;
    epd_asset_font_asset_glyph_node_t cur = size_node->value.glyph_list;
    while (cur && cur->value.codepoint < key.codepoint) {
        pre = cur;
        cur = cur->next;
    }
    if (!cur || cur->value.codepoint != key.codepoint) {
        return EPD_OK;
    }

    if (pre) {
        pre->next = cur->next;
    } else {
        size_node->value.glyph_list = cur->next;
    }
    epd_asset_font_asset_glyph_destroy(&cur->value);
    free(cur);
    return EPD_OK;
}

bool epd_asset_font_asset_contains_glyph(const epd_asset_font_asset_t asset,
    epd_asset_font_asset_glyph_key_t key)
{
    if (!asset || key.size == 0U) {
        return false;
    }

    epd_asset_font_asset_size_node_t size_node = epd_asset_font_asset_find_size(asset, key.size);
    if (!size_node) {
        return false;
    }

    return epd_asset_font_asset_find_glyph(&size_node->value, key.codepoint) != NULL;
}

epd_err_t epd_asset_font_asset_write_egf(const epd_asset_font_asset_t asset, epd_stream_t* stream)
{
    if (!asset || !stream) {
        return EPD_ERR_INVALID_ARG;
    }

    epd_asset_font_asset_stats_t stats = epd_asset_font_asset_stats(asset);
    epd_err_t                  ret   = EPD_OK;

    EPD_CHECK_RET(epd_asset_font_asset_write_header(stream, stats.size_count, stats.glyph_count, stats.data_count));
    EPD_CHECK_RET(epd_asset_font_asset_write_size_table(asset, stream));
    EPD_CHECK_RET(epd_asset_font_asset_write_glyph_index_table(asset, stream));
    return epd_asset_font_asset_write_glyph_data_table(asset, stream);
}
