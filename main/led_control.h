#ifndef LED_CONTROL_H
#define LED_CONTROL_H

#include <stdint.h>  
#include "led_strip.h"

// 定义LED引脚和LED数量
#define LED_PIN  21               // WS2812连接的GPIO引脚
#define LED_COUNT 144              // LED条的LED数量

#define  LED_COUNT_2  144        
#define LED_PIN_2 38             

void led_init(void);
void set_led_color(led_strip_handle_t strip, uint8_t r, uint8_t g, uint8_t b);
void set_white_led_strip(void);


#endif // LED_CONTROL_H
