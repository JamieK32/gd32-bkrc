#include "key.h"
#include "delay.h"
#include "usart_lcd.h"
#include "freertos.h"
#include "task.h"
#include "stdio.h"
#include "main.h"


/*********************
 *      创建4个按键对象
 *********************/
ButtonController btn1 = {.port = KEY_1_GPIO_Port, .pin = KEY_1_Pin, .pin_last_state = 1, .count = 0};
ButtonController btn2 = {.port = KEY_2_GPIO_Port, .pin = KEY_2_Pin, .pin_last_state = 1, .count = 0};
ButtonController btn3 = {.port = KEY_3_GPIO_Port, .pin = KEY_3_Pin, .pin_last_state = 1, .count = 0};
ButtonController btn4 = {.port = KEY_4_GPIO_Port, .pin = KEY_4_Pin, .pin_last_state = 1, .count = 0};


/*********************
 *      函数指针赋值
 *********************/
key_i key = {
    .init = KEY_Init,
    .read_all = read_all_keys,
};


static void KEY_Init(void) {
  rcu_periph_clock_enable(KEY_1_RTC);
  rcu_periph_clock_enable(KEY_2_RTC);
  rcu_periph_clock_enable(KEY_3_RTC);
  rcu_periph_clock_enable(KEY_4_RTC);

  gpio_mode_set(KEY_1_GPIO_Port, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, KEY_1_Pin);
  gpio_mode_set(KEY_2_GPIO_Port, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, KEY_2_Pin);
  gpio_mode_set(KEY_3_GPIO_Port, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, KEY_3_Pin);
  gpio_mode_set(KEY_4_GPIO_Port, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, KEY_4_Pin);
}

static void Clicked(ButtonController *btn) {
    uint8_t state = 0;
    if (btn == &btn1) state |= (1 << 0);
    if (btn == &btn2) state |= (1 << 1);
    if (btn == &btn3) state |= (1 << 2);
    if (btn == &btn4) state |= (1 << 3);
    if (key.click_cb != NULL) key.click_cb(state);
}

/*********************
 *      读取单个引脚，您不用阅读这里的逻辑
 *********************/
static void read_key(ButtonController *btn) {
  if (btn == NULL) {
    LOG_E("btn is null");
  }
  btn->current_millis = (unsigned int)xTaskGetTickCount();
  btn->pin_current_state = KEY_STATE(btn->port, btn->pin);
  if (btn->pin_current_state != btn->pin_last_state) {
    if (btn->debounce_flag == 0) {
      btn->previous_millis = btn->current_millis;
      btn->debounce_flag = 1;
    } else if ((btn->current_millis - btn->previous_millis) >= DEBOUNCE_DELAY) {
      if (btn->pin_current_state == GPIO_PIN_RESET) {
        btn->isPressed = 1;
        btn->isReleased = 0;
        btn->count++;
        Clicked(btn);
      } else if (btn->pin_current_state == GPIO_PIN_SET) {
        btn->isReleased = 1;
        btn->isPressed = 0;
      }
      btn->pin_last_state = btn->pin_current_state;
      btn->debounce_flag = 0;
    }
  } else {
    btn->debounce_flag = 0;
  }
}


/*********************
 *      循环中调用读取所有引脚
 *********************/
static uint8_t read_all_keys(void) 
{
    uint8_t state = 0;

    read_key(&btn1);
    read_key(&btn2);
    read_key(&btn3);
    read_key(&btn4);

    if (btn1.isPressed) state |= (1 << 0);
    if (btn2.isPressed) state |= (1 << 1);
    if (btn3.isPressed) state |= (1 << 2);
    if (btn4.isPressed) state |= (1 << 3);
    #if DEBUG
        taskENTER_CRITICAL();
        LOG_I("key num = %d", state);
        taskEXIT_CRITICAL();
    #endif

  return state;
}

