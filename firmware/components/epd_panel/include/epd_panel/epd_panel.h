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

/**
 * @brief Creates a new e-paper panel instance based on the configuration.
 * 
 * @param cfg Configuration settings for the e-paper panel.
 * @param panel Pointer to the panel handle to be initialized.
 * @return `ESP_OK` on success, or an error code from `esp_err_t` if the operation fails.
 */
esp_err_t epd_panel_create(const epd_panel_cfg_t* cfg, epd_panel_t* panel);

/**
 * @brief Initializes the e-paper panel, setting up communication and panel configuration.
 * 
 * @param panel The handle of the e-paper panel to be initialized.
 * @return `ESP_OK` on success, or an error code from `esp_err_t` if the operation fails.
 */
esp_err_t epd_panel_init(epd_panel_t panel);

/**
 * @brief Puts the e-paper panel into a low-power sleep state.
 * 
 * @param panel The handle of the e-paper panel to be put to sleep.
 * @return `ESP_OK` on success, or an error code from `esp_err_t` if the operation fails.
 */
esp_err_t epd_panel_sleep(epd_panel_t panel);

/**
 * @brief Destroys the e-paper panel instance, releasing allocated resources.
 * 
 * @param panel The handle of the e-paper panel to be destroyed.
 * @return `ESP_OK` on success, or an error code from `esp_err_t` if the operation fails.
 */
esp_err_t epd_panel_destroy(epd_panel_t panel);

/**
 * @brief Fills the entire e-paper display with the specified color.
 * The color is applied to all pixels on the display.
 * 
 * @param panel The handle of the e-paper panel.
 * @param color The color to fill the display (black, white, or red).
 * @return `ESP_OK` on success, or an error code from `esp_err_t` if the operation fails.
 */
esp_err_t epd_panel_fill(epd_panel_t panel, epd_panel_color_t color);

/**
 * @brief Clears the e-paper display, resetting all pixels to the default color (usually white).
 * 
 * @param panel The handle of the e-paper panel to be cleared.
 * @return `ESP_OK` on success, or an error code from `esp_err_t` if the operation fails.
 */
esp_err_t epd_panel_clear(epd_panel_t panel);

/**
 * @brief Sends image data to the e-paper panel for display.
 * The input data is in native format, where 2 pixels are packed into one byte (4bpp).
 * 
 * @param panel The handle of e-paper panel.
 * @param data Image data in native format (4bpp).
 * @param size Size of data. The size must match the expected buffer size: stride * height.
 * @return `ESP_OK` on success, or an error code from `esp_err_t` if the operation fails.
 */
esp_err_t epd_panel_show(epd_panel_t panel, const void* data, uint32_t size);

/**
 * @brief Sends image data to the e-paper panel for display.
 * The input data is in plane format, which consists of two planes: black and red.
 * Each byte in the plane represents 8 pixels, with each pixel encoded as black, white, or red.
 * 
 * @param panel The handle of the e-paper panel.
 * @param pblk Pointer to the black plane image data (1bpp).
 * @param pred Pointer to the red plane image data (1bpp).
 * @param size The size of the input data. The size must be equal to stride * height.
 * @return `ESP_OK` on success, or an error code from `esp_err_t` if the operation fails.
 */
esp_err_t epd_panel_show_planes(epd_panel_t panel, const void* pblk,
    const void* pred, uint32_t size);

#endif // _EPD_PANEL_H_