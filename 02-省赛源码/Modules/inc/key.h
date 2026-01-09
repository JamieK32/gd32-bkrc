#ifndef __key_H
#define __key_H

#include "gd32f4xx.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "delay.h"

#define KEY_1_RTC RCU_GPIOE
#define KEY_1_GPIO_Port GPIOE
#define KEY_1_Pin GPIO_PIN_0

#define KEY_2_RTC RCU_GPIOE
#define KEY_2_GPIO_Port GPIOE
#define KEY_2_Pin GPIO_PIN_1

#define KEY_3_RTC RCU_GPIOE
#define KEY_3_GPIO_Port GPIOE
#define KEY_3_Pin GPIO_PIN_2

#define KEY_4_RTC RCU_GPIOE
#define KEY_4_GPIO_Port GPIOE
#define KEY_4_Pin GPIO_PIN_3

#define KEY1_PRES 1
#define KEY2_PRES 2
#define KEY3_PRES 3
#define KEY4_PRES 4

#define KEY1 gpio_input_bit_get(KEY_1_GPIO_Port, KEY_1_Pin)
#define KEY2 gpio_input_bit_get(KEY_2_GPIO_Port, KEY_2_Pin)
#define KEY3 gpio_input_bit_get(KEY_3_GPIO_Port, KEY_3_Pin)
#define KEY4 gpio_input_bit_get(KEY_4_GPIO_Port, KEY_4_Pin)

/*********************
 *      移植宏定义
 *********************/
#define KEY_STATE(GPIO_PORT, GPIO_PIN) (gpio_input_bit_get(GPIO_PORT, GPIO_PIN))
#define GPIO_PIN_RESET 0
#define GPIO_PIN_SET 1
#define DEBOUNCE_DELAY 20

/*********************
 *      单个按键控制管理
 *********************/
typedef struct ButtonControler
{
	uint32_t port;
	uint32_t pin;
	uint32_t current_millis;
	uint32_t previous_millis;
	uint8_t pin_last_state;
	uint8_t pin_current_state;
	uint8_t debounce_flag;
	uint16_t count;
	uint8_t isPressed;
	uint8_t isReleased;
} ButtonController;

/*********************
 *      按键接口
 *********************/
typedef struct key_i {
    void (*init)(void);
    uint8_t (*read_all)(void);
    void (*click_cb)(uint8_t);
} key_i;

static void KEY_Init(void);
static void read_key(ButtonController *btn);
static uint8_t read_all_keys(void);

extern key_i key;

extern ButtonController btn1;
extern ButtonController btn2;
extern ButtonController btn3;
extern ButtonController btn4;

#endif
