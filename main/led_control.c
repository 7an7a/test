#include "led_control.h"
#include "led_strip.h"
#include "esp_log.h"

led_strip_handle_t led_strip;
led_strip_handle_t led_strip_2;

static const char *TAG = "LED";


void led_init(void) {
    ESP_LOGI(TAG, "Initializing LED...");
    
    // 初始化彩虹灯条
    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_PIN,
        .max_leds = LED_COUNT,
        .led_model = LED_MODEL_WS2812,
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
        .flags = {.invert_out = false},
    };

    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000,
        .mem_block_symbols = 64,
        .flags = {.with_dma = false},
    };

    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    ESP_LOGI(TAG, "First LED strip (rainbow) initialized successfully");

    // 初始化白色灯条
    led_strip_config_t strip_config_2 = {
        .strip_gpio_num = LED_PIN_2,
        .max_leds = LED_COUNT_2,
        .led_model = LED_MODEL_WS2812,
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
        .flags = {.invert_out = false},
    };

    led_strip_rmt_config_t rmt_config_2 = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000,
        .mem_block_symbols = 64,
        .flags = {.with_dma = false},
    };

    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config_2, &rmt_config_2, &led_strip_2));
    ESP_LOGI(TAG, "Second LED strip (white) initialized successfully");

    // 检查是否成功初始化
    if (led_strip_2 == NULL) {
        ESP_LOGE(TAG, "Failed to initialize second LED strip (white)!");
    }
}

// 设置颜色
void set_led_color(led_strip_handle_t strip, uint8_t r, uint8_t g, uint8_t b) {
    for (int i = 0; i < LED_COUNT; i++) {
        ESP_ERROR_CHECK(led_strip_set_pixel(strip, i, r, g, b));
    }
    ESP_ERROR_CHECK(led_strip_refresh(strip));
    ESP_ERROR_CHECK(led_strip_refresh(led_strip_2));  // 刷新 LED 灯条
}

// 设置白色灯条
void set_white_led_strip(void) {
    ESP_LOGI(TAG, "Setting white color on second LED strip...");

    // 设置第二根灯条为白色
    for (int i = 0; i < LED_COUNT_2; i++) {
        ESP_ERROR_CHECK(led_strip_set_pixel(led_strip_2, i, 255, 255, 255));  // 白色
    }

    // 刷新灯条
    ESP_ERROR_CHECK(led_strip_refresh(led_strip_2));
}
