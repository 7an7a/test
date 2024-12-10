#ifndef WS2812_H
#define WS2812_H

#include <stdint.h>

// WS2812 LED 控制相关函数声明
void ws2812_init(void);                       // 初始化 WS2812
void ws2812_set_color(uint8_t r, uint8_t g, uint8_t b); // 设置 RGB 灯的颜色
void ws2812_display_status(const char *status);         // 根据状态控制灯光颜色

#endif // WS2812_H
