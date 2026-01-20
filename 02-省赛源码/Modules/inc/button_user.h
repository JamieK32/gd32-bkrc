#ifndef __key_H
#define __key_H

#include "gd32f4xx.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "delay.h"
#include "multi_button.h"

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



typedef enum {
	KEY_A = 0,
	KEY_B,
	KEY_C,
	KEY_D,
	BUTTON_NUM,
} BUTTON_ID;

void multi_button_init(BtnCallback single_click_cb);
void multi_button_register_callbacks(BtnCallback long_press_cb);

extern struct _Button buttons[BUTTON_NUM];


#endif
