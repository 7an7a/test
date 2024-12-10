#include "ws2812.h"
#include "esp_log.h"
#include "driver/gpio.h"  // 用于 GPIO 控制
#include <string.h>  // 用于 strcmp 函数
#include <stdio.h>
#include <math.h>
#include "led_strip.h" 


#define LED_PIN     21 // 请根据实际硬件设置引脚
#define NUM_LEDS    60  // 假设LED条上有60个LED灯

static const char *TAG = "WS2812";
// 色彩结构体
typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} rgb_color_t;

void ws2812_init(void) {
    ESP_LOGI(TAG, "Initializing WS2812...");
    // 配置 WS2812 的 GPIO 引脚
    // 使用 ESP32 的 RMT 外设或其他库进行初始化
    // 示例：将 GPIO 设置为输出
    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
}

void ws2812_set_color(uint8_t r, uint8_t g, uint8_t b) {
    ESP_LOGI(TAG, "Setting color: R=%d, G=%d, B=%d", r, g, b);
    // 示例：发送颜色数据到 WS2812
    // 使用你熟悉的库（比如 NeoPixel 或 RMT）发送颜色
    gpio_set_level(LED_PIN, (r > 0 || g > 0 || b > 0)); // 简单逻辑演示
}

void ws2812_display_status(const char *status) {
    if (strcmp(status, "correct") == 0) {
        ws2812_set_color(0, 255, 0); // 绿色
    } else if (strcmp(status, "incorrect") == 0) {
        ws2812_set_color(255, 0, 0); // 红色
    } else if (strcmp(status, "invalid Params") == 0) {
        ws2812_set_color(0, 0, 255); // 蓝色
    } else {
        ws2812_set_color(0, 0, 0);   // 关灯
    }
}
