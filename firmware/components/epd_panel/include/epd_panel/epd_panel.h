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
#include <driver/spi_master.h>
#include <epd_core/common.h>
#include <epd_gfx/common.h>
#include <epd_gfx/frame_view.h>

#ifdef __cplusplus
extern "C" {
#endif

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

/**
 * @brief Create a new e-paper panel instance based on the configuration.
 * 
 * @param cfg Configuration settings for the e-paper panel.
 * @param panel Pointer to the panel handle to be initialized.
 * @return `EPD_OK` on success, or an error code from `epd_err_t` if the operation fails.
 */
epd_err_t epd_panel_create(const epd_panel_cfg_t* cfg, epd_panel_t* panel);

/**
 * @brief Initialize the e-paper panel, setting up communication and panel configuration.
 * 
 * @param panel The handle of the e-paper panel to be initialized.
 * @return `EPD_OK` on success, or an error code from `epd_err_t` if the operation fails.
 */
epd_err_t epd_panel_init(epd_panel_t panel);

/**
 * @brief Put the e-paper panel into a low-power sleep state.
 * 
 * @param panel The handle of the e-paper panel to be put to sleep.
 * @return `EPD_OK` on success, or an error code from `epd_err_t` if the operation fails.
 */
epd_err_t epd_panel_sleep(epd_panel_t panel);

/**
 * @brief Destroy the e-paper panel instance, releasing allocated resources.
 * 
 * @param panel The handle of the e-paper panel to be destroyed.
 * @return `EPD_OK` on success, or an error code from `epd_err_t` if the operation fails.
 */
epd_err_t epd_panel_destroy(epd_panel_t panel);

/**
 * @brief Fill the entire e-paper display with the specified color.
 * The color is applied to all pixels on the display.
 * 
 * @param panel The handle of the e-paper panel.
 * @param color The color to fill the display (black, white, or red).
 * @return `EPD_OK` on success, or an error code from `epd_err_t` if the operation fails.
 */
epd_err_t epd_panel_fill(epd_panel_t panel, epd_gfx_color_t color);

/**
 * @brief Clear the e-paper display, resetting all pixels to the default color (usually white).
 * 
 * @param panel The handle of the e-paper panel to be cleared.
 * @return `EPD_OK` on success, or an error code from `epd_err_t` if the operation fails.
 */
epd_err_t epd_panel_clear(epd_panel_t panel);

/**
 * @brief Send image data in native format to the e-paper panel for display.
 * 
 * @param panel The handle of e-paper panel.
 * @param data Image data in native format.
 * @param size Size of data. The size must match the expected buffer size: stride * height.
 * @return `EPD_OK` on success, or an error code from `epd_err_t` if the operation fails.
 */
epd_err_t epd_panel_show(epd_panel_t panel, const uint8_t* data, uint32_t size);

/**
 * @brief Send image data in white/red planes format to the e-paper panel for display.
 * 
 * @remark
 * +---+---+-------+
 * | r | w | color |
 * +---+---+-------+
 * | 0 | 0 | black |
 * | 0 | 1 | white |
 * | 1 | - |  red  |
 * +---+---+-------+
 * 
 * @param panel The handle of the e-paper panel.
 * @param pwht Pointer to the white plane image data.
 * @param pred Pointer to the red plane image data.
 * @param size The size of the input data. The size must be equal to stride * height.
 * @return `EPD_OK` on success, or an error code from `epd_err_t` if the operation fails.
 */
epd_err_t epd_panel_show_planes(epd_panel_t panel, const uint8_t* pwht,
    const uint8_t* pred, uint32_t size);

/**
 * @brief Build a frame sink bound to the panel for `epd_gfx_canvas_flush`.
 *
 * @param panel The handle of the e-paper panel.
 * @param sink Output sink to fill; must not be null.
 * @note Ownership of the allocated sink is transferred to the caller.
 * @return `EPD_OK` on success, otherwise an error code from `epd_err_t`.
 */
epd_err_t epd_panel_make_sink(epd_panel_t panel, epd_gfx_frame_view_sink_t** sink);

#ifdef __cplusplus
}
#endif

#endif // _EPD_PANEL_H_
