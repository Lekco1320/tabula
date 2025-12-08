/**
 * @file epd_panel.c
 * @brief Driver for 7.5\" tri-color e-paper (DEPG0750* UC8159).
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2025-11-13
 * @license MIT
 */

#include <string.h>
#include <esp_timer.h>
#include <driver/spi_master.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <epd_gfx/codec.h>

#include "epd_panel/epd_panel.h"

#define EPD_CMD_PANEL_SETTING      0x00    // PSR
#define EPD_CMD_POWER_SETTING      0x01    // PWR
#define EPD_CMD_POWER_OFF          0x02    // POF
#define EPD_CMD_POWER_OFF_SEQ      0x03    // PFS
#define EPD_CMD_POWER_ON           0x04    // PON
#define EPD_CMD_BOOSTER_SOFT_START 0x06    // BTST
#define EPD_CMD_DEEP_SLEEP         0x07    // DSLP
#define EPD_CMD_DTM1               0x10    // Data Start Transmission 1
#define EPD_CMD_DATA_STOP          0x11    // DSP
#define EPD_CMD_DISPLAY_REFRESH    0x12    // DRF
#define EPD_CMD_IPC                0x13    // Image Process Command
#define EPD_CMD_PLL_CONTROL        0x30    // PLL
#define EPD_CMD_TSE_SETTING        0x41    // TSE
#define EPD_CMD_VCOM_INTERVAL      0x50    // CDI
#define EPD_CMD_TCON_SETTING       0x60    // TCON
#define EPD_CMD_TCON_RESOLUTION    0x61    // TRES
#define EPD_CMD_VCOM_DC            0x82    // VCM_DC
#define EPD_CMD_FLASH_MODE         0xE5    // Flash mode

#define EPD_BUSY_TIMEOUT_MS        20000   // 20s

#define TAG "epd_panel"

struct epd_panel_impl {
    epd_panel_cfg_t     cfg;
    spi_device_handle_t spi;
    bool                spi_bus_inited;
};

static epd_err_t epd_wait_idle(epd_panel_t panel, int timeout_ms)
{
    if (panel->cfg.pin_busy < 0) {
        return EPD_OK;
    }

    int64_t start = esp_timer_get_time() / 1000; // us -> ms
    while (true) {
        int level = gpio_get_level(panel->cfg.pin_busy);
        if (level == 1) {
            return EPD_OK;
        }

        int64_t now = esp_timer_get_time() / 1000;
        if ((now - start) > timeout_ms) {
            return EPD_ERR_TIMEOUT;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

static epd_err_t epd_panel_reset(epd_panel_t panel)
{
    if (panel->cfg.pin_reset < 0) {
        return EPD_OK;
    }

    gpio_set_level(panel->cfg.pin_reset, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(panel->cfg.pin_reset, 0);
    vTaskDelay(pdMS_TO_TICKS(200));
    gpio_set_level(panel->cfg.pin_reset, 1);
    vTaskDelay(pdMS_TO_TICKS(10));

    return EPD_OK;
}

static epd_err_t epd_send_command(epd_panel_t panel, uint8_t cmd)
{
    gpio_set_level(panel->cfg.pin_dc, 0); // command

    spi_transaction_t t = {
        .flags  = SPI_TRANS_USE_TXDATA,
        .length = 8,
    };
    t.tx_data[0] = cmd;

    return spi_device_transmit(panel->spi, &t);
}

static epd_err_t epd_send_data(epd_panel_t panel, const void* data, size_t len)
{
    if (len == 0) {
        return EPD_OK;
    }

    gpio_set_level(panel->cfg.pin_dc, 1); // data

    spi_transaction_t t = {
        .length    = len * 8,
        .tx_buffer = data,
    };
    return spi_device_transmit(panel->spi, &t);
}

epd_err_t epd_panel_create(const epd_panel_cfg_t* cfg, epd_panel_t* out_panel)
{
    if (!cfg || !out_panel) {
        return EPD_ERR_INVALID_ARG;
    }
    if (cfg->width == 0 || cfg->height == 0) {
        return EPD_ERR_INVALID_ARG;
    }
    if (!GPIO_IS_VALID_OUTPUT_GPIO(cfg->pin_dc) ||
        !GPIO_IS_VALID_OUTPUT_GPIO(cfg->pin_cs) ||
        !GPIO_IS_VALID_OUTPUT_GPIO(cfg->pin_mosi) ||
        !GPIO_IS_VALID_OUTPUT_GPIO(cfg->pin_sclk) ||
        !GPIO_IS_VALID_OUTPUT_GPIO(cfg->pin_reset) ||
        !GPIO_IS_VALID_GPIO(cfg->pin_busy)) {
        return EPD_ERR_INVALID_ARG;
    }

    epd_err_t ret = EPD_OK;
    epd_panel_t panel = calloc(1, sizeof(struct epd_panel_impl));
    if (!panel) {
        return EPD_ERR_NO_MEM;
    }
    panel->cfg = *cfg;

    // Configure GPIO:
    // Output: RESET, DC
    // Input:  BUSY
    uint64_t out_mask = (1ULL << cfg->pin_reset) | (1ULL << cfg->pin_dc);
    gpio_config_t io_out = {
        .pin_bit_mask = out_mask,
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    EPD_CHECK_GOTO(gpio_config(&io_out), fail);

    gpio_config_t io_in = {
        .pin_bit_mask = 1ULL << cfg->pin_busy,
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    EPD_CHECK_GOTO(gpio_config(&io_in), fail);

    // Set default
    gpio_set_level(cfg->pin_reset, 1);
    gpio_set_level(cfg->pin_dc, 0);

    // initialize SPI bus
    spi_bus_config_t buscfg = {
        .mosi_io_num     = cfg->pin_mosi,
        .miso_io_num     = -1,
        .sclk_io_num     = cfg->pin_sclk,
        .quadwp_io_num   = -1,
        .quadhd_io_num   = -1,
        .max_transfer_sz = (uint32_t)cfg->width * cfg->height / 2 + 16,
    };
    EPD_CHECK_GOTO(spi_bus_initialize(cfg->spi_host, &buscfg, SPI_DMA_CH_AUTO), fail);
    panel->spi_bus_inited = true;

    // mount SPI device
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 10 * 1000 * 1000, // 10MHz
        .mode           = 0,
        .spics_io_num   = cfg->pin_cs,
        .queue_size     = 4,
        .flags          = SPI_DEVICE_HALFDUPLEX,
    };
    EPD_CHECK_GOTO(spi_bus_add_device((spi_host_device_t)cfg->spi_host, &devcfg, &panel->spi), fail);

    *out_panel = panel;
    return EPD_OK;

fail:
    if (panel->spi) {
        spi_bus_remove_device(panel->spi);
    }
    if (panel->spi_bus_inited) {
        spi_bus_free(cfg->spi_host);
    }
    free(panel);
    return ret;
}

epd_err_t epd_panel_init(epd_panel_t panel)
{
    if (!panel) {
        return EPD_ERR_INVALID_ARG;
    }

    // 1. HW reset
    epd_err_t ret = EPD_OK;
    EPD_CHECK_RET(epd_panel_reset(panel));

    // 2. Booster soft start
    {
        uint8_t data[] = { 0xC7, 0xCC, 0x28 };
        EPD_CHECK_RET(epd_send_command(panel, EPD_CMD_BOOSTER_SOFT_START));
        EPD_CHECK_RET(epd_send_data(panel, data, sizeof(data)));
    }

    // 3. Power setting
    {
        uint8_t data[] = { 0x37, 0x00 };
        EPD_CHECK_RET(epd_send_command(panel, EPD_CMD_POWER_SETTING));
        EPD_CHECK_RET(epd_send_data(panel, data, sizeof(data)));
    }

    // 4. Power on
    EPD_CHECK_RET(epd_send_command(panel, EPD_CMD_POWER_ON));
    EPD_CHECK_RET(epd_wait_idle(panel, EPD_BUSY_TIMEOUT_MS));

    // 5. Panel setting
    {
        uint8_t data[] = { 0xCF, 0x08 };
        EPD_CHECK_RET(epd_send_command(panel, EPD_CMD_PANEL_SETTING));
        EPD_CHECK_RET(epd_send_data(panel, data, sizeof(data)));
    }

    // 6. PLL control
    {
        uint8_t data[] = { 0x3C };
        EPD_CHECK_RET(epd_send_command(panel, EPD_CMD_PLL_CONTROL));
        EPD_CHECK_RET(epd_send_data(panel, data, sizeof(data)));
    }

    // 7. VCOM & data interval setting (CDI)
    {
        uint8_t data[] = { 0x77 };
        EPD_CHECK_RET(epd_send_command(panel, EPD_CMD_VCOM_INTERVAL));
        EPD_CHECK_RET(epd_send_data(panel, data, sizeof(data)));
    }

    // 8. TCON setting
    {
        uint8_t data[] = { 0x22 };
        EPD_CHECK_RET(epd_send_command(panel, EPD_CMD_TCON_SETTING));
        EPD_CHECK_RET(epd_send_data(panel, data, sizeof(data)));
    }

    // 9. Resolution setting (TRES)
    {
        uint8_t data[4];
        const epd_panel_cfg_t* cfg = &panel->cfg;
        data[0] = (cfg->width  >> 8);    // HRES[9:2]
        data[1] = (cfg->width  &  0xFF); // HRES[1:0] + dummy
        data[2] = (cfg->height >> 8);    // VRES[8:1]
        data[3] = (cfg->height &  0xFF); // VRES[0] + dummy

        EPD_CHECK_RET(epd_send_command(panel, EPD_CMD_TCON_RESOLUTION));
        EPD_CHECK_RET(epd_send_data(panel, data, sizeof(data)));
    }

    // 10. VCM_DC setting
    {
        uint8_t data[] = { 0x1E };
        EPD_CHECK_RET(epd_send_command(panel, EPD_CMD_VCOM_DC));
        EPD_CHECK_RET(epd_send_data(panel, data, sizeof(data)));
    }

    // 11. Temperature calibration
    {
        uint8_t data[] = { 0x00 };
        EPD_CHECK_RET(epd_send_command(panel, EPD_CMD_TSE_SETTING));
        EPD_CHECK_RET(epd_send_data(panel, data, sizeof(data)));
    }

    // 12. Flash mode
    {
        uint8_t data[] = { 0x03 };
        EPD_CHECK_RET(epd_send_command(panel, EPD_CMD_FLASH_MODE));
        EPD_CHECK_RET(epd_send_data(panel, data, sizeof(data)));
    }

    return EPD_OK;
}

epd_err_t epd_panel_sleep(epd_panel_t panel)
{
    if (!panel) {
        return EPD_ERR_INVALID_ARG;
    }

    epd_err_t ret;
    EPD_CHECK_RET(epd_send_command(panel, EPD_CMD_POWER_OFF));
    EPD_CHECK_RET(epd_wait_idle(panel, EPD_BUSY_TIMEOUT_MS));

    uint8_t data = 0xA5;
    EPD_CHECK_RET(epd_send_command(panel, EPD_CMD_DEEP_SLEEP));
    return epd_send_data(panel, &data, 1);
}

epd_err_t epd_panel_destroy(epd_panel_t panel)
{
    if (!panel) {
        return EPD_ERR_INVALID_ARG;
    }

    if (panel->spi) {
        (void)spi_bus_remove_device(panel->spi);
    }
    if (panel->spi_bus_inited) {
        (void)spi_bus_free(panel->cfg.spi_host);
    }

    free(panel);
    return EPD_OK;
}

epd_err_t epd_panel_fill(epd_panel_t panel, epd_gfx_color_t color)
{
    if (!panel) {
        return EPD_ERR_INVALID_ARG;
    }

    const uint16_t height     = panel->cfg.height;
    const uint32_t stride     = epd_gfx_native_stride(panel->cfg.width);
    const uint8_t  color_byte = epd_gfx_pack_colors(color, color);

    uint8_t* chunk = (uint8_t*)malloc(stride);
    if (!chunk) {
        return EPD_ERR_NO_MEM;
    }
    memset(chunk, color_byte, stride);

    epd_err_t ret = EPD_OK;
    EPD_CHECK_GOTO(epd_send_command(panel, EPD_CMD_DTM1), fail);

    for (uint16_t row = 0; row < height; ++row) {
        EPD_CHECK_GOTO(epd_send_data(panel, chunk, stride), fail);
    }

    // Data stop and refresh the display
    uint8_t data = 0x80;
    EPD_CHECK_GOTO(epd_send_command(panel, EPD_CMD_DATA_STOP), fail);
    EPD_CHECK_GOTO(epd_send_data(panel, &data, 1), fail);
    EPD_CHECK_GOTO(epd_send_command(panel, EPD_CMD_DISPLAY_REFRESH), fail);

    free(chunk);
    return epd_wait_idle(panel, EPD_BUSY_TIMEOUT_MS);

fail:
    if (chunk) {
        free(chunk);
    }
    return ret;
}

epd_err_t epd_panel_clear(epd_panel_t panel)
{
    return epd_panel_fill(panel, EPD_GFX_WHITE);
}

epd_err_t epd_panel_show(epd_panel_t panel, const uint8_t* data, uint32_t size)
{
    if (!panel || !data) {
        return EPD_ERR_INVALID_ARG;
    }

    const uint16_t height = panel->cfg.height;
    const uint32_t stride = epd_gfx_native_stride(panel->cfg.width);
    const uint32_t length = stride * panel->cfg.height;
    if (size != length) {
        return EPD_ERR_INVALID_SIZE;
    }

    epd_err_t      ret = EPD_OK;
    const uint8_t* ptr = data;
    EPD_CHECK_RET(epd_send_command(panel, EPD_CMD_DTM1));
    for (uint16_t row = 0; row < height; ++row) {
        EPD_CHECK_RET(epd_send_data(panel, (void*)ptr, stride));
        ptr += stride;
    }

    uint8_t flag = 0x80;
    EPD_CHECK_RET(epd_send_command(panel, EPD_CMD_DATA_STOP));
    EPD_CHECK_RET(epd_send_data(panel, &flag, 1));
    EPD_CHECK_RET(epd_send_command(panel, EPD_CMD_DISPLAY_REFRESH));

    return epd_wait_idle(panel, EPD_BUSY_TIMEOUT_MS);
}

epd_err_t epd_panel_show_planes(epd_panel_t panel, const uint8_t* pwht,
    const uint8_t* pred, uint32_t size)
{
    if (!panel || !pwht || !pred) {
        return EPD_ERR_INVALID_ARG;
    }

    const uint16_t height       = panel->cfg.height;
    const uint32_t plane_stride = epd_gfx_planes_stride(panel->cfg.width);
    const uint32_t plane_length = plane_stride * height;
    if (size != plane_length) {
        return EPD_ERR_INVALID_SIZE;
    }

    const uint32_t native_stride = epd_gfx_native_stride(panel->cfg.width);
    uint8_t* buffer = (uint8_t*)calloc(native_stride, sizeof(uint8_t));
    if (!buffer) {
        return EPD_ERR_NO_MEM;
    }

    epd_err_t ret = EPD_OK;
    EPD_CHECK_GOTO(epd_send_command(panel, EPD_CMD_DTM1), fail);
    
    for (uint16_t prow = 0; prow < height; ++prow) {
        uint32_t index = prow * plane_stride;
        epd_gfx_planes_to_native_buffer(pwht + index, pred + index, panel->cfg.width, buffer);
        ret = epd_send_data(panel, buffer, native_stride);
        if (ret != EPD_OK) {
            EPD_CHECK_GOTO(epd_send_command(panel, EPD_CMD_DATA_STOP), fail);
            goto fail;
        }
    }

    // Data stop and refresh the display
    uint8_t data = 0x80;
    EPD_CHECK_GOTO(epd_send_command(panel, EPD_CMD_DATA_STOP), fail);
    EPD_CHECK_GOTO(epd_send_data(panel, &data, 1), fail);
    EPD_CHECK_GOTO(epd_send_command(panel, EPD_CMD_DISPLAY_REFRESH), fail);

    free(buffer);
    return epd_wait_idle(panel, EPD_BUSY_TIMEOUT_MS);

fail:
    free(buffer);
    return ret;
}

static epd_err_t epd_panel_flush_impl(void* context, const epd_gfx_frame_view_t* frame_view)
{
    epd_panel_t panel = (epd_panel_t)context;
    if (!panel || !frame_view) {
        return EPD_ERR_INVALID_ARG;
    }

    uint32_t size = frame_view->height * frame_view->stride;
    switch (frame_view->format)
    {
    case EPD_GFX_FORMAT_NATIVE:
        return epd_panel_show(panel, frame_view->buf_native, size);
    
    case EPD_GFX_FORMAT_PLANES:
        return epd_panel_show_planes(panel, frame_view->buf_wht, frame_view->buf_red, size);

    default:
        return EPD_ERR_INVALID_ARG;
    }
}

epd_err_t epd_panel_make_sink(epd_panel_t panel, epd_gfx_frame_view_sink_t** sink)
{
    if (!panel || !sink) {
        return EPD_ERR_INVALID_ARG;
    }

    epd_gfx_frame_view_sink_t* new_sink = (epd_gfx_frame_view_sink_t*)calloc(1, sizeof(epd_gfx_frame_view_sink_t));
    new_sink->context    = panel;
    new_sink->flush_impl = epd_panel_flush_impl;

    *sink = new_sink;
    return EPD_OK;
}