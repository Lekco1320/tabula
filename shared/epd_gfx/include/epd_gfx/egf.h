/**
 * @file egf.h
 * @brief EGF1 font file format helpers.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2026-05-13
 * @license MIT
 */

#pragma once

#ifndef _EPD_GFX_EGF_H_
#define _EPD_GFX_EGF_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <epd_core/common.h>
#include <epd_core/stream.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EPD_GFX_EGF_MAGIC             "EGF1"
#define EPD_GFX_EGF_MAGIC_BYTES       4U
#define EPD_GFX_EGF_HEADER_BYTES      16U
#define EPD_GFX_EGF_SIZE_TABLE_OFFSET EPD_GFX_EGF_HEADER_BYTES
#define EPD_GFX_EGF_SIZE_RECORD_BYTES 20U
#define EPD_GFX_EGF_GLYPH_INDEX_BYTES 18U

/**
 * @brief EGF1 file header.
 */
typedef struct {
    char     magic[4];
    uint32_t size_count;
    uint32_t glyph_count;
    uint32_t data_count;
} epd_gfx_egf_header_t;

/**
 * @brief EGF1 size table record.
 */
typedef struct {
    uint16_t size;
    int16_t  ascent;
    int16_t  descent;
    int16_t  line_height;
    uint32_t glyph_count;
    uint32_t glyph_index_offset;
    uint32_t glyph_data_offset;
} epd_gfx_egf_size_record_t;

/**
 * @brief EGF1 glyph index table record.
 */
typedef struct {
    uint32_t codepoint;
    uint16_t width;
    uint16_t height;
    int16_t  xoffset;
    int16_t  yoffset;
    int16_t  advance;
    uint32_t data_offset;
} epd_gfx_egf_glyph_index_t;

/**
 * @brief Read exactly `size` bytes from a stream.
 *
 * @param stream Source stream.
 * @param dst Destination buffer.
 * @param size Number of bytes to read.
 * @return true if exactly `size` bytes were read, otherwise false.
 */
bool epd_gfx_egf_stream_read_exact(const epd_stream_t* stream, void* dst, size_t size);

/**
 * @brief Check whether an EGF header contains the EGF1 magic.
 *
 * @param header Header to validate.
 * @return true if the magic is `EGF1`, otherwise false.
 */
bool epd_gfx_egf_check_magic(const epd_gfx_egf_header_t* header);

/**
 * @brief Read an EGF1 header from the current stream position.
 *
 * @param stream Source stream.
 * @param header Header buffer to fill.
 * @return true on success, otherwise false.
 */
bool epd_gfx_egf_read_header(const epd_stream_t* stream, epd_gfx_egf_header_t* header);

/**
 * @brief Read an EGF1 size record from the current stream position.
 *
 * @param stream Source stream.
 * @param record Size record buffer to fill.
 * @return true on success, otherwise false.
 */
bool epd_gfx_egf_read_size_record(const epd_stream_t* stream, epd_gfx_egf_size_record_t* record);

/**
 * @brief Read an EGF1 glyph index record from the current stream position.
 *
 * @param stream Source stream.
 * @param record Glyph index record buffer to fill.
 * @return true on success, otherwise false.
 */
bool epd_gfx_egf_read_glyph_index(const epd_stream_t* stream, epd_gfx_egf_glyph_index_t* record);

/**
 * @brief Seek to a size record by index.
 *
 * @param stream Source stream.
 * @param index Size record index.
 * @return true on success, otherwise false.
 */
bool epd_gfx_egf_seek_size_record(const epd_stream_t* stream, uint32_t index);

/**
 * @brief Read a size record by index.
 *
 * @param stream Source stream.
 * @param index Size record index.
 * @param record Size record buffer to fill.
 * @return `EPD_OK` on success, otherwise an error code from `epd_err_t`.
 */
epd_err_t epd_gfx_egf_read_size_record_at(const epd_stream_t* stream, uint32_t index,
    epd_gfx_egf_size_record_t* record);

/**
 * @brief Seek to a glyph index record by base offset and record index.
 *
 * @param stream Source stream.
 * @param base_offset Glyph index table base offset.
 * @param index Glyph index record index.
 * @return true on success, otherwise false.
 */
bool epd_gfx_egf_seek_glyph_index(const epd_stream_t* stream, uint32_t base_offset, uint32_t index);

/**
 * @brief Read a glyph index record by base offset and record index.
 *
 * @param stream Source stream.
 * @param base_offset Glyph index table base offset.
 * @param index Glyph index record index.
 * @param record Glyph index record buffer to fill.
 * @return `EPD_OK` on success, otherwise an error code from `epd_err_t`.
 */
epd_err_t epd_gfx_egf_read_glyph_index_at(const epd_stream_t* stream, uint32_t base_offset,
    uint32_t index, epd_gfx_egf_glyph_index_t* record);

#ifdef __cplusplus
}
#endif

#endif // !_EPD_GFX_EGF_H_
