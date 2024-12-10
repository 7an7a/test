#ifndef RFID_H
#define RFID_H

#include "driver/gpio.h"
#include "esp_log.h"
#include "rc522.h"
#include "rc522_spi.h"
#include "rc522_picc.h"

// 定义宏和常量
#define LED_GREEN_GPIO             (10)  // 绿色 LED
#define LED_RED_GPIO               (38)  // 红色 LED
#define LED_BLUE_GPIO              (21)  // 蓝色 LED
#define BUZZER_GPIO                (42)  // 蜂鸣器

#define RC522_SPI_BUS_GPIO_MISO    (11)
#define RC522_SPI_BUS_GPIO_MOSI    (12)
#define RC522_SPI_BUS_GPIO_SCLK    (14)
#define RC522_SPI_SCANNER_GPIO_SDA (13)
#define RC522_SCANNER_GPIO_RST     (-1) // soft-reset

#define RC522_PICC_UID_MAX_SIZE    10  // 最大UID大小

// 函数声明
void rfid_main(void);  // 主应用函数，用于初始化和启动扫描
void buzzer_beep(int duration_ms);  // 蜂鸣器鸣响
void set_rgb_led_color(uint8_t red, uint8_t green, uint8_t blue);  // 设置RGB LED颜色
void rainbow_led_effect(void *arg);  // 平缓的彩虹灯效函数

// 卡片状态变化事件处理函数
void on_picc_state_changed(void *arg, esp_event_base_t base, int32_t event_id, void *data);

extern const char *TAG_rfid;  // 记录日志的TAG

#endif // RFID_H
