/*
 * SPDX-FileCopyrightText: 2023-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led_strip.h"
#include "esp_log.h"
#include "esp_err.h"

// GPIO assignment
#define LED_STRIP_GPIO_PIN  21
// Number of LEDs in the strip
#define LED_STRIP_LED_COUNT 288
// 10MHz resolution, 1 tick = 0.1us (LED strip needs a high resolution)
#define LED_STRIP_RMT_RES_HZ  (10 * 1000 * 1000)

static const char *TAG = "example";

// RGB color structure
typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} rgb_color_t;

// HSV to RGB conversion function
rgb_color_t hsv_to_rgb(uint16_t h, uint8_t s, uint8_t v) {
    uint8_t r, g, b;
    uint16_t region, remainder, p, q, t;

    if (s == 0) {
        r = g = b = v;  // If saturation is 0, it's a gray color
        return (rgb_color_t){r, g, b};
    }

    region = h / 60;          // Determine the region of the hue (360° -> 6 sections)
    remainder = h - (region * 60);  // Find remainder of the hue within the region
    p = (v * (255 - s)) / 255;  // Calculate the RGB components
    q = (v * (255 - (s * remainder) / 60)) / 255;
    t = (v * (255 - (s * (60 - remainder)) / 60)) / 255;

    switch (region) {
        case 0:
            r = v; g = t; b = p; break;
        case 1:
            r = q; g = v; b = p; break;
        case 2:
            r = p; g = v; b = t; break;
        case 3:
            r = p; g = q; b = v; break;
        case 4:
            r = t; g = p; b = v; break;
        case 5:
            r = v; g = p; b = q; break;
        default:
            r = g = b = 0;  // Error case
    }

    return (rgb_color_t){r, g, b};
}

// LED strip configuration function
led_strip_handle_t configure_led(void)
{
    // LED strip general initialization, according to your led board design
    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_STRIP_GPIO_PIN, // The GPIO that connected to the LED strip's data line
        .max_leds = LED_STRIP_LED_COUNT,      // The number of LEDs in the strip,
        .led_model = LED_MODEL_WS2812,        // LED strip model
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB, // The color order of the strip: GRB
        .flags = {
            .invert_out = false, // don't invert the output signal
        }
    };

    // LED strip backend configuration: RMT
    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,        // different clock source can lead to different power consumption
        .resolution_hz = LED_STRIP_RMT_RES_HZ, // RMT counter clock frequency
        .mem_block_symbols = 64,              // the memory size of each RMT channel, in words (4 bytes)
        .flags = {
            .with_dma = false, // DMA feature is available on chips like ESP32-S3/P4
        }
    };

    // LED strip object handle
    led_strip_handle_t led_strip;
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    ESP_LOGI(TAG, "Created LED strip object with RMT backend");
    return led_strip;
}

// Rainbow effect function (with continuously changing colors)
void rainbow_effect(led_strip_handle_t led_strip, uint16_t hue_offset) {
    // Iterate through each LED and set its color to create a rainbow effect
    for (int i = 0; i < LED_STRIP_LED_COUNT; i++) {
        uint16_t hue = (i * 360 / LED_STRIP_LED_COUNT + hue_offset) % 360; // Apply offset to make the effect flow
        rgb_color_t color = hsv_to_rgb(hue, 255, 255); // Convert HSV to RGB
        ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, i, color.r, color.g, color.b)); // Set LED color
    }

    // Refresh the strip to send the data
    ESP_ERROR_CHECK(led_strip_refresh(led_strip));
}

// Main application entry point
void app_main(void)
{
    led_strip_handle_t led_strip = configure_led();
    uint16_t hue_offset = 0; // The hue offset to shift the colors

    ESP_LOGI(TAG, "Start running the dynamic rainbow effect");

    while (1) {
        rainbow_effect(led_strip, hue_offset); // Run the dynamic rainbow effect
        hue_offset += 1; // Gradually change the hue offset for the next frame

        // Make sure the offset stays within the valid range (0 to 360)
        if (hue_offset >= 360) {
            hue_offset = 0;
        }

        vTaskDelay(pdMS_TO_TICKS(50)); // Delay for 50 ms to create the flowing effect
    }
}
