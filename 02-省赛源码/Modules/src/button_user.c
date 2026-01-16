#include "button_user.h"
#include "delay.h"
#include "freertos.h"
#include "task.h"
#include "stdio.h"
#include "main.h"

#define KEY_ACTIVE_LEVEL 0

struct _Button buttons[BUTTON_NUM]; 

uint8_t button_ids[BUTTON_NUM] = {KEY_A, KEY_B, KEY_C, KEY_D};

static uint8_t read_key0(void) { return gpio_input_bit_get(KEY_1_GPIO_Port, KEY_1_Pin); }
static uint8_t read_key1(void) { return gpio_input_bit_get(KEY_2_GPIO_Port, KEY_2_Pin); }
static uint8_t read_key2(void) { return gpio_input_bit_get(KEY_3_GPIO_Port, KEY_3_Pin); }
static uint8_t read_key3(void) { return gpio_input_bit_get(KEY_4_GPIO_Port, KEY_4_Pin); }


/* 你的按钮读取函数：适配 button_init 的 read_cb */
static inline uint8_t read_button_GPIO(uint8_t button_id)
{
    switch (button_id)
    {
        case KEY_A: return read_key0();
        case KEY_B: return read_key1();
        case KEY_C: return read_key2();
        case KEY_D: return read_key3();
        default:    return 0;
    }
}

void muti_button_init(BtnCallback single_click_cb)
{
		rcu_periph_clock_enable(KEY_1_RTC);
		rcu_periph_clock_enable(KEY_2_RTC);
		rcu_periph_clock_enable(KEY_3_RTC);
		rcu_periph_clock_enable(KEY_4_RTC);

		gpio_mode_set(KEY_1_GPIO_Port, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, KEY_1_Pin);
		gpio_mode_set(KEY_2_GPIO_Port, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, KEY_2_Pin);
		gpio_mode_set(KEY_3_GPIO_Port, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, KEY_3_Pin);
		gpio_mode_set(KEY_4_GPIO_Port, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, KEY_4_Pin);
		
    for (int i = 0; i < BUTTON_NUM; i++)
    {
        button_init(&buttons[i], read_button_GPIO, KEY_ACTIVE_LEVEL, button_ids[i]);
        button_attach(&buttons[i], BTN_SINGLE_CLICK, single_click_cb);
        button_start(&buttons[i]);
    }
}
