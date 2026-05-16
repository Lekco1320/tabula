/**
 * @file epd_vfs.h
 * @brief Board-side VFS stream adapter.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2026-05-16
 * @license MIT
 */

#pragma once

#ifndef _EPD_VFS_H_
#define _EPD_VFS_H_

#include <epd_core/common.h>
#include <epd_core/stream.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EPD_VFS_ASSETS_PATH "/assets"
#define EPD_VFS_FONTS_PATH  EPD_VFS_ASSETS_PATH "/fonts"

/**
 * @brief Mount project VFS partitions.
 *
 * @return `EPD_OK` on success, or an error code from `epd_err_t` if the operation fails.
 */
epd_err_t epd_vfs_mount(void);

/**
 * @brief Unmount project VFS partitions.
 *
 * @return `EPD_OK` on success, or an error code from `epd_err_t` if the operation fails.
 */
epd_err_t epd_vfs_unmount(void);

/**
 * @brief Open a VFS file as an `epd_stream_t`.
 *
 * @param path Absolute VFS path.
 * @param out_stream Pointer to the stream to initialize.
 * @return `EPD_OK` on success, or an error code from `epd_err_t` if the operation fails.
 */
epd_err_t epd_vfs_open_file(const char* path, epd_stream_t* out_stream);

/**
 * @brief Close a stream opened by `epd_vfs_open_file`.
 *
 * @param stream Stream to close.
 * @return `EPD_OK` on success, or an error code from `epd_err_t` if the operation fails.
 */
epd_err_t epd_vfs_close_file(epd_stream_t* stream);

#ifdef __cplusplus
}
#endif

#endif // _EPD_VFS_H_
