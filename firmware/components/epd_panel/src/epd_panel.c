/**
 * @file epd_panel.h
 * @brief Driver for 7.5\" tri-color e-paper (DEPG0750* UC8159).
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2025-11-13
 * @license MIT
 */

#include <esp_log.h>
#include <esp_err.h>
#include <esp_timer.h>
#include <driver/spi_master.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "epd_panel.h"

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

#define EPD_CHECK_GOTO(EXP, LABEL) \
    ret = (EXP); \
    if (ret != ESP_OK) { \
        goto LABEL; \
    }

#define EPD_CHECK_RET(EXP) \
    ret = (EXP); \
    if (ret != ESP_OK) { \
        return ret; \
    }

struct epd_panel_impl {
    epd_panel_cfg_t     cfg;
    spi_device_handle_t spi;
};

static esp_err_t epd_wait_idle(epd_panel_t panel, int timeout_ms)
{
    if (panel->cfg.pin_busy < 0) {
        return ESP_OK;
    }

    int64_t start = esp_timer_get_time() / 1000; // us -> ms
    while (true) {
        int level = gpio_get_level(panel->cfg.pin_busy);
        if (level == 1) {
            return ESP_OK;
        }

        int64_t now = esp_timer_get_time() / 1000;
        if ((now - start) > timeout_ms) {
            return ESP_ERR_TIMEOUT;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

static esp_err_t epd_panel_reset(epd_panel_t panel)
{
    if (panel->cfg.pin_reset < 0) {
        return ESP_OK;
    }

    gpio_set_level(panel->cfg.pin_reset, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(panel->cfg.pin_reset, 0);
    vTaskDelay(pdMS_TO_TICKS(200));
    gpio_set_level(panel->cfg.pin_reset, 1);
    vTaskDelay(pdMS_TO_TICKS(10));

    return ESP_OK;
}

static esp_err_t epd_send_command(epd_panel_t panel, uint8_t cmd)
{
    gpio_set_level(panel->cfg.pin_dc, 0); // command

    spi_transaction_t t = {
        .flags  = SPI_TRANS_USE_TXDATA,
        .length = 8,
    };
    t.tx_data[0] = cmd;

    return spi_device_transmit(panel->spi, &t);
}

static esp_err_t epd_send_data(epd_panel_t panel, const void* data, size_t len)
{
    if (len == 0) {
        return ESP_OK;
    }

    gpio_set_level(panel->cfg.pin_dc, 1); // data

    spi_transaction_t t = {
        .length    = len * 8,
        .tx_buffer = data,
    };
    return spi_device_transmit(panel->spi, &t);
}

esp_err_t epd_panel_create(const epd_panel_cfg_t* cfg, epd_panel_t* out_panel)
{
    if (!cfg || !out_panel) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret = ESP_OK;
    epd_panel_t panel = calloc(1, sizeof(struct epd_panel_impl));
    if (!panel) {
        return ESP_ERR_NO_MEM;
    }
    panel->cfg = *cfg;

    // Configure GPIO:
    // Output: RESET, DC
    // Input:  BUSY
    uint64_t out_mask = 0;
    if (cfg->pin_reset >= 0) {
        out_mask |= 1ULL << cfg->pin_reset;
    }
    if (cfg->pin_dc >= 0) {
        out_mask |= 1ULL << cfg->pin_dc;
    }

    if (out_mask) {
        gpio_config_t io_out = {
            .pin_bit_mask = out_mask,
            .mode         = GPIO_MODE_OUTPUT,
            .pull_up_en   = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type    = GPIO_INTR_DISABLE,
        };
        EPD_CHECK_GOTO(gpio_config(&io_out), fail);
    }

    if (cfg->pin_busy >= 0) {
        gpio_config_t io_in = {
            .pin_bit_mask = 1ULL << cfg->pin_busy,
            .mode         = GPIO_MODE_INPUT,
            .pull_up_en   = GPIO_PULLUP_ENABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type    = GPIO_INTR_DISABLE,
        };
        EPD_CHECK_GOTO(gpio_config(&io_in), fail);
    }

    // Set default
    if (cfg->pin_reset >= 0) {
        gpio_set_level(cfg->pin_reset, 1);
    }
    if (cfg->pin_dc >= 0) {
        gpio_set_level(cfg->pin_dc, 0);
    }

    // initialize SPI bus
    spi_bus_config_t buscfg = {
        .mosi_io_num = cfg->pin_mosi,
        .miso_io_num = -1,
        .sclk_io_num = cfg->pin_sclk,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = (uint32_t)cfg->width * cfg->height / 2 + 16,
    };
    EPD_CHECK_GOTO(spi_bus_initialize(cfg->spi_host, &buscfg, SPI_DMA_CH_AUTO), fail);

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
    ESP_LOGI(TAG, "EPD panel created: %dx%d", cfg->width, cfg->height);
    return ESP_OK;

fail:
    if (panel->spi) {
        spi_bus_remove_device(panel->spi);
    }
    free(panel);
    return ret;
}

esp_err_t epd_panel_init(epd_panel_t panel)
{
    if (!panel) {
        return ESP_ERR_INVALID_ARG;
    }

    // 1. HW reset
    esp_err_t ret = ESP_OK;
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

    return ESP_OK;
}

esp_err_t epd_panel_sleep(epd_panel_t panel)
{
    if (!panel) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret;
    EPD_CHECK_RET(epd_send_command(panel, EPD_CMD_POWER_OFF));
    EPD_CHECK_RET(epd_wait_idle(panel, EPD_BUSY_TIMEOUT_MS));

    uint8_t data = 0xA5;
    EPD_CHECK_RET(epd_send_command(panel, EPD_CMD_DEEP_SLEEP));
    return epd_send_data(panel, &data, 1);
}

esp_err_t epd_panel_destroy(epd_panel_t panel)
{
    if (!panel) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret = ESP_OK;
    if (panel->spi) {
        ret = spi_bus_remove_device(panel->spi);
    }

    free(panel);
    ESP_LOGI(TAG, "EPD panel destroyed");
    return ret;
}

esp_err_t epd_panel_fill(epd_panel_t panel, epd_panel_color_t color)
{
    if (!panel) {
        return ESP_ERR_INVALID_ARG;
    }

    const uint32_t pixels      = (uint32_t)panel->cfg.width * panel->cfg.height;
    const uint32_t total_bytes = (pixels + 1) / 2;
    const uint8_t  color_byte  = ((color << 4) | (color & 0x0F)); // duplicate in one byte

    uint8_t chunk[256];
    memset(chunk, color_byte, sizeof(chunk));

    esp_err_t ret = ESP_OK;
    EPD_CHECK_RET(epd_send_command(panel, EPD_CMD_DTM1));

    uint32_t remaining = total_bytes;
    while (remaining > 0) {
        size_t n = (remaining > sizeof(chunk) ? sizeof(chunk) : remaining);
        EPD_CHECK_RET(epd_send_data(panel, chunk, n));
        remaining -= n;
    }

    // Data stop and refresh the display
    uint8_t data = 0x80;
    EPD_CHECK_RET(epd_send_command(panel, EPD_CMD_DATA_STOP));
    EPD_CHECK_RET(epd_send_data(panel, &data, 1));
    EPD_CHECK_RET(epd_send_command(panel, EPD_CMD_DISPLAY_REFRESH));

    return epd_wait_idle(panel, EPD_BUSY_TIMEOUT_MS);
}

esp_err_t epd_panel_clear(epd_panel_t panel)
{
    return epd_panel_fill(panel, EPD_PANEL_WHITE);
}

esp_err_t epd_panel_show(epd_panel_t panel, const void* data, size_t size)
{
    if (!panel) {
        return ESP_ERR_INVALID_ARG;
    }

    const uint32_t pixels      = (uint32_t)panel->cfg.width * panel->cfg.height;
    const uint32_t total_bytes = (pixels + 1) / 2;
    if (size != total_bytes) {
        return ESP_ERR_INVALID_SIZE;
    }

    esp_err_t ret       = ESP_OK;
    void*     ptr       = (void*)data;
    uint32_t  remaining = total_bytes;
    EPD_CHECK_RET(epd_send_command(panel, EPD_CMD_DTM1));
    while (remaining > 0) {
        size_t n = (remaining > 256 ? 256 : remaining);
        EPD_CHECK_RET(epd_send_data(panel, (void*)ptr, n));
        remaining -= n;
        ptr       += n;
    }

    uint8_t flag = 0x80;
    EPD_CHECK_RET(epd_send_command(panel, EPD_CMD_DATA_STOP));
    EPD_CHECK_RET(epd_send_data(panel, &flag, 1));
    EPD_CHECK_RET(epd_send_command(panel, EPD_CMD_DISPLAY_REFRESH));

    return epd_wait_idle(panel, EPD_BUSY_TIMEOUT_MS);
}