/* rgb_led_sm.h */
#ifndef __RGB_LED_SM_H__
#define __RGB_LED_SM_H__

#include "gd32f4xx.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdint.h>
#include <stdbool.h>

/* === GPIO 定义：连接 P24（你原来用的是 PB3/PB4/PB5） === */
#define LED_B_RTC        RCU_GPIOB
#define LED_B_GPIO_Port  GPIOB
#define LED_B_Pin        GPIO_PIN_3

#define LED_G_RTC        RCU_GPIOB
#define LED_G_GPIO_Port  GPIOB
#define LED_G_Pin        GPIO_PIN_4

#define LED_R_RTC        RCU_GPIOB
#define LED_R_GPIO_Port  GPIOB
#define LED_R_Pin        GPIO_PIN_5



/* 颜色：用枚举替代宏 */
typedef enum {
    RGB_LED_COLOR_OFF = 0,
    RGB_LED_COLOR_RED,
    RGB_LED_COLOR_GREEN,
    RGB_LED_COLOR_BLUE,
    RGB_LED_COLOR_CYAN,
    RGB_LED_COLOR_YELLOW,
    RGB_LED_COLOR_PURPLE,
    RGB_LED_COLOR_WHITE
} rgb_led_color_t;

/* 初始化 */
void rgb_led_init(void);

rgb_led_color_t rgb_led_get_color_from_rgb(uint16_t r, uint16_t g, uint16_t b);

/* 基础控制（立即生效，进入“常亮”模式） */
void rgb_led_off(void);
void rgb_led_set_rgb(uint8_t r, uint8_t g, uint8_t b);     /* 0/1 */
void rgb_led_set_color(rgb_led_color_t color);

/* 效果控制：全部由 rgb_led_ticks() 驱动 */
void rgb_led_pwm_start(rgb_led_color_t color, float duty_percent, float freq_hz);
void rgb_led_pwm_stop(void);

void rgb_led_blink_start(rgb_led_color_t color, uint16_t period_ms); /* period=0 等同 off */
void rgb_led_breath_start(rgb_led_color_t color, uint16_t step_ms);  /* 典型 15ms */
void rgb_led_test_start(uint16_t step_ms);                           /* 典型 500ms */

/* 状态机 tick：把它放到你的某个 task 里循环调用（建议 1ms~10ms 一次） */
void rgb_led_ticks(void);

#endif /* __RGB_LED_SM_H__ */
