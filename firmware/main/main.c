#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "esp_log.h"
#include "led_strip.h"
#include <epd_core/common.h>
#include <epd_core/stream.h>
#include <epd_gfx/bitmap.h>
#include <epd_gfx/canvas.h>
#include <epd_gfx/font.h>
#include <epd_gfx/text.h>
#include <epd_panel/epd_panel.h>
#include <epd_vfs/epd_vfs.h>

// GPIO assignment
#define LED_STRIP_BLINK_GPIO 48
// Numbers of the LED in the strip
#define LED_STRIP_LED_NUMBERS 1
// 10MHz resolution, 1 tick = 0.1us (led strip needs a high resolution)
#define LED_STRIP_RMT_RES_HZ  (10 * 1000 * 1000)

led_strip_handle_t configure_led(void)
{
    // LED strip general initialization, according to your led board design
    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_STRIP_BLINK_GPIO,   // The GPIO that connected to the LED strip's data line
        .max_leds = LED_STRIP_LED_NUMBERS,        // The number of LEDs in the strip,
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB, // Pixel format of your LED strip
        .led_model = LED_MODEL_WS2812,            // LED strip model
        .flags.invert_out = false,                // whether to invert the output signal
    };
    led_strip_rmt_config_t rmt_config = {
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
        .rmt_channel = 0,
#else
        .clk_src = RMT_CLK_SRC_DEFAULT,        // different clock source can lead to different power consumption
        .resolution_hz = LED_STRIP_RMT_RES_HZ, // RMT counter clock frequency
        .flags.with_dma = false,               // DMA feature is available on ESP target like ESP32-S3
#endif
    };

    // LED Strip object handle
    led_strip_handle_t led_strip;
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    return led_strip;
}

void print_params(void)
{
    led_strip_handle_t led_strip = configure_led();
    uint8_t brightness = 50;
    uint8_t red = (0 * brightness) / 100;
    uint8_t green = (0 * brightness) / 100;
    uint8_t blue = (139 * brightness) / 100;
    led_strip_set_pixel(led_strip, 0, red, green, blue);
    led_strip_refresh(led_strip);

    printf("Hello world!\n");

    /* Print chip information */
    esp_chip_info_t chip_info;
    uint32_t flash_size;
    esp_chip_info(&chip_info);
    printf("This is %s chip with %d CPU core(s), %s%s%s%s, ",
           CONFIG_IDF_TARGET,
           chip_info.cores,
           (chip_info.features & CHIP_FEATURE_WIFI_BGN) ? "WiFi/" : "",
           (chip_info.features & CHIP_FEATURE_BT) ? "BT" : "",
           (chip_info.features & CHIP_FEATURE_BLE) ? "BLE" : "",
           (chip_info.features & CHIP_FEATURE_IEEE802154) ? ", 802.15.4 (Zigbee/Thread)" : "");

    unsigned major_rev = chip_info.revision / 100;
    unsigned minor_rev = chip_info.revision % 100;
    printf("silicon revision v%d.%d, ", major_rev, minor_rev);
    if(esp_flash_get_size(NULL, &flash_size) != ESP_OK) {
        printf("Get flash size failed");
        return;
    }

    printf("%" PRIu32 "MB %s flash\n", flash_size / (uint32_t)(1024 * 1024),
           (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    printf("Minimum free heap size: %" PRIu32 " bytes\n", esp_get_minimum_free_heap_size());

    for (int i = 10; i >= 0; i--) {
        printf("Restarting in %d seconds...\n", i);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    printf("Restarting now.\n");
    fflush(stdout);
    esp_restart();
}

void app_main(void)
{
    led_strip_handle_t led_strip = configure_led();
    uint8_t brightness = 0;
    uint8_t red = (0 * brightness) / 100;
    uint8_t green = (0 * brightness) / 100;
    uint8_t blue = (139 * brightness) / 100;
    led_strip_set_pixel(led_strip, 0, red, green, blue);
    led_strip_refresh(led_strip);

    epd_panel_cfg_t config = {
        .pin_reset = GPIO_NUM_2,
        .pin_dc    = GPIO_NUM_3,
        .pin_busy  = GPIO_NUM_4,
        .pin_cs    = GPIO_NUM_10,
        .pin_mosi  = GPIO_NUM_11,
        .pin_sclk  = GPIO_NUM_12,
        .spi_host  = SPI2_HOST,
        .width     = 640,
        .height    = 384,
    };

    epd_err_t   ret   = EPD_OK;
    epd_panel_t panel = NULL;
    ret = epd_panel_create(&config, &panel);
    if (ret != EPD_OK) {
        ESP_LOGE("epd_test", "EPD panel creation failed! err=%s", epd_err_to_str(ret));
        return;
    } else {
        ESP_LOGI("epd_test", "EPD panel created! addr:0x%x", panel);
    }

    ret = epd_panel_init(panel);
    if (ret != EPD_OK) {
        ESP_LOGE("epd_test", "EPD panel initialization failed! err=%s", epd_err_to_str(ret));
        goto clean_panel;
    } else {
        ESP_LOGI("epd_test", "EPD panel initialized");
    }

    epd_gfx_canvas_config_t canvas_config = {
        .width    = 640,
        .height   = 384,
        .format   = EPD_GFX_FORMAT_NATIVE,
        .rotation = EPD_GFX_ROTATE_90,
    };
    epd_gfx_canvas_t canvas = NULL;
    ret = epd_gfx_canvas_create(&canvas_config, &canvas);
    if (ret != EPD_OK) {
        ESP_LOGE("epd_test", "EPD canvas create failed! err=%s", epd_err_to_str(ret));
        goto clean_panel;
    } else {
        ESP_LOGI("epd_test", "EPD canvas created!");
    }

    epd_gfx_frame_view_sink_t sink = epd_panel_make_sink(panel);
    if (!sink.context || !sink.flush_impl) {
        ESP_LOGE("epd_test", "EPD panel sink create failed!");
        goto clean_canvas;
    } else {
        ESP_LOGI("epd_test", "EPD panel sink created!");
    }

    epd_stream_t     bitmap_stream      = { 0 };
    epd_stream_t     system_font_stream = { 0 };
    epd_gfx_bitmap_t tangyuan_bitmap    = NULL;
    epd_gfx_font_t   system_font        = NULL;
    bool             vfs_mounted        = false;

    epd_gfx_canvas_fill(canvas, EPD_GFX_WHITE);

    ret = epd_vfs_mount();
    if (ret != EPD_OK) {
        ESP_LOGE("epd_test", "EPD VFS mount failed! err=%s", epd_err_to_str(ret));
        goto clean_fonts;
    }
    vfs_mounted = true;

    ret = epd_vfs_open_file(EPD_VFS_BITMAPS_PATH "/tangyuan.ebm", &bitmap_stream);
    if (ret != EPD_OK) {
        ESP_LOGE("epd_test", "Open tangyuan bitmap failed! err=%s", epd_err_to_str(ret));
        goto clean_fonts;
    }

    ret = epd_gfx_bitmap_load(&bitmap_stream, &tangyuan_bitmap);
    if (ret != EPD_OK) {
        ESP_LOGE("epd_test", "Load tangyuan bitmap failed! err=%s", epd_err_to_str(ret));
        goto clean_fonts;
    }

    ret = epd_gfx_canvas_draw_bitmap(canvas, tangyuan_bitmap, (epd_gfx_point_t){ 1, 1 });
    if (ret != EPD_OK) {
        ESP_LOGE("epd_test", "Draw tangyuan bitmap failed! err=%s", epd_err_to_str(ret));
        goto clean_fonts;
    }

    ret = epd_vfs_open_file(EPD_VFS_FONTS_PATH "/system.egf", &system_font_stream);
    if (ret != EPD_OK) {
        ESP_LOGE("epd_test", "Open system font failed! err=%s", epd_err_to_str(ret));
        goto clean_fonts;
    }

    ret = epd_gfx_font_load(&system_font_stream, &system_font);
    if (ret != EPD_OK) {
        ESP_LOGE("epd_test", "Load system font failed! err=%s", epd_err_to_str(ret));
        goto clean_fonts;
    }

    epd_gfx_font_size_info_t size_info = { 0 };
    ret = epd_gfx_font_get_size_info(system_font, 16, &size_info);
    if (ret != EPD_OK) {
        ESP_LOGE("epd_test", "Get system font size failed! err=%s", epd_err_to_str(ret));
        goto clean_fonts;
    }

    epd_gfx_text_box_style_t text_style = {
        .text = {
            .size           = 16,
            .color          = EPD_GFX_BLACK,
            .background     = EPD_GFX_BG_WHITE,
            .letter_spacing = 0,
        },
        .align        = EPD_GFX_TEXT_ALIGN_CENTER,
        .line_spacing = 0,
        .wrap         = false,
    };
    ret = epd_gfx_canvas_draw_utf8_box(canvas, system_font, "TANGYUAN",
        (epd_gfx_rect_t){ 1, 450, epd_gfx_canvas_get_logical_width(canvas) - 2,
            (uint16_t)size_info.line_height }, &text_style);
    if (ret != EPD_OK) {
        ESP_LOGE("epd_test", "Draw text failed! err=%s", epd_err_to_str(ret));
        goto clean_fonts;
    }

    ret = epd_gfx_canvas_flush(canvas, &sink);
    if (ret != EPD_OK) {
        ESP_LOGE("epd_test", "EPD canvas flush failed! err=%s", epd_err_to_str(ret));
        goto clean_fonts;
    } else {
        ESP_LOGI("epd_test", "EPD canvas flush flushed!");
    }

    ret = epd_panel_sleep(panel);
    if (ret != EPD_OK) {
        ESP_LOGE("epd_test", "EPD panel sleeping failed! err=%s", epd_err_to_str(ret));
    } else {
        ESP_LOGI("epd_test", "EPD panel slept!");
    }

clean_fonts:
    (void)epd_gfx_bitmap_destroy(tangyuan_bitmap);
    (void)epd_gfx_font_destroy(system_font);
    if (bitmap_stream.ctx) {
        (void)epd_vfs_close_file(&bitmap_stream);
    }
    if (system_font_stream.ctx) {
        (void)epd_vfs_close_file(&system_font_stream);
    }
    if (vfs_mounted) {
        (void)epd_vfs_unmount();
    }

clean_canvas:
    (void)epd_gfx_canvas_destroy(canvas);

clean_panel:
    (void)epd_panel_destroy(panel);
}
