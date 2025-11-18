/**
 * @file epd_panel.h
 * @brief Driver for 7.5\" tri-color e-paper (DEPG0750* UC8159).
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2025-11-13
 * @license MIT
 */

#pragma once

#ifndef _EPD_PANEL_H_
#define _EPD_PANEL_H_

#include <driver/gpio.h>

typedef struct epd_panel_impl* epd_panel_t;

typedef struct {
    gpio_num_t        pin_reset; // PIN Reset
    gpio_num_t        pin_dc;    // PIN DC
    gpio_num_t        pin_busy;  // PIN BUSY
    gpio_num_t        pin_cs;    // PIN CS
    gpio_num_t        pin_mosi;  // PIN MOSI
    gpio_num_t        pin_sclk;  // PIN SCLK
    spi_host_device_t spi_host;  // HSPI_HOST / VSPI_HOST

    uint16_t          width;
    uint16_t          height;
} epd_panel_cfg_t;

typedef enum {
    EPD_PANEL_BLACK = 0b000,
    EPD_PANEL_WHITE = 0b011,
    EPD_PANEL_RED   = 0b100,
} epd_panel_color_t;

esp_err_t epd_panel_create(const epd_panel_cfg_t* cfg, epd_panel_t* panel);
esp_err_t epd_panel_init(epd_panel_t panel);
esp_err_t epd_panel_sleep(epd_panel_t panel);
esp_err_t epd_panel_destroy(epd_panel_t panel);
esp_err_t epd_panel_fill(epd_panel_t panel, epd_panel_color_t color);
esp_err_t epd_panel_clear(epd_panel_t panel);
esp_err_t epd_panel_show(epd_panel_t panel, const void* data, size_t size);

#endif // _EPD_PANEL_H_