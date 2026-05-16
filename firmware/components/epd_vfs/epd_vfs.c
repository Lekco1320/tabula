/**
 * @file epd_vfs.c
 * @brief Board-side VFS stream adapter.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2026-05-16
 * @license MIT
 */

#include <stdio.h>
#include <string.h>
#include <esp_err.h>
#include <esp_spiffs.h>
#include <epd_vfs/epd_vfs.h>

static size_t epd_vfs_file_read(void* ctx, void* dst, size_t size)
{
    return fread(dst, 1U, size, (FILE*)ctx);
}

static size_t epd_vfs_file_write(void* ctx, const void* src, size_t size)
{
    return fwrite(src, 1U, size, (FILE*)ctx);
}

static bool epd_vfs_file_seek(void* ctx, int64_t offset, epd_seek_whence_t whence)
{
    int origin = SEEK_SET;
    switch (whence)
    {
    case EPD_SEEK_SET:
        origin = SEEK_SET;
        break;

    case EPD_SEEK_CUR:
        origin = SEEK_CUR;
        break;

    case EPD_SEEK_END:
        origin = SEEK_END;
        break;

    default:
        return false;
    }

    return fseek((FILE*)ctx, (long)offset, origin) == 0;
}

static int64_t epd_vfs_file_tell(void* ctx)
{
    long position = ftell((FILE*)ctx);
    if (position < 0) {
        return -1;
    }
    return (int64_t)position;
}

static int64_t epd_vfs_file_size(void* ctx)
{
    FILE* file     = (FILE*)ctx;
    long  position = ftell(file);
    if (position < 0) {
        return -1;
    }

    if (fseek(file, 0L, SEEK_END) != 0) {
        return -1;
    }

    long size = ftell(file);
    if (fseek(file, position, SEEK_SET) != 0 || size < 0) {
        return -1;
    }

    return (int64_t)size;
}

static bool epd_vfs_file_eof(void* ctx)
{
    return feof((FILE*)ctx) != 0;
}

static int32_t epd_vfs_file_error(void* ctx)
{
    return ferror((FILE*)ctx);
}

static bool epd_vfs_file_close(void* ctx)
{
    return fclose((FILE*)ctx) == 0;
}

epd_err_t epd_vfs_mount(void)
{
    esp_vfs_spiffs_conf_t conf = {
        .base_path              = EPD_VFS_ASSETS_PATH,
        .partition_label        = "assets",
        .max_files              = 8,
        .format_if_mount_failed = false,
    };

    esp_err_t err = esp_vfs_spiffs_register(&conf);
    if (err == ESP_OK || err == ESP_ERR_INVALID_STATE) {
        return EPD_OK;
    }
    return EPD_FAIL;
}

epd_err_t epd_vfs_unmount(void)
{
    esp_err_t err = esp_vfs_spiffs_unregister("assets");
    if (err == ESP_OK || err == ESP_ERR_INVALID_STATE) {
        return EPD_OK;
    }
    return EPD_FAIL;
}

epd_err_t epd_vfs_open_file(const char* path, epd_stream_t* out_stream)
{
    if (!path || path[0] != '/' || !out_stream) {
        return EPD_ERR_INVALID_ARG;
    }

    FILE* file = fopen(path, "rb");
    if (!file) {
        return EPD_ERR_NOT_FOUND;
    }

    memset(out_stream, 0, sizeof(*out_stream));
    out_stream->ctx   = file;
    out_stream->read  = epd_vfs_file_read;
    out_stream->write = epd_vfs_file_write;
    out_stream->seek  = epd_vfs_file_seek;
    out_stream->tell  = epd_vfs_file_tell;
    out_stream->size  = epd_vfs_file_size;
    out_stream->eof   = epd_vfs_file_eof;
    out_stream->error = epd_vfs_file_error;
    out_stream->close = epd_vfs_file_close;
    return EPD_OK;
}

epd_err_t epd_vfs_close_file(epd_stream_t* stream)
{
    if (!stream || !stream->ctx || !stream->close) {
        return EPD_ERR_INVALID_ARG;
    }

    bool closed = stream->close(stream->ctx);
    memset(stream, 0, sizeof(*stream));
    return closed ? EPD_OK : EPD_FAIL;
}
