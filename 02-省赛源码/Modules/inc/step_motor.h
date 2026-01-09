#ifndef __STEP_H_
#define __STEP_H_

#include "gd32f4xx.h"
#include "delay.h"

#define STEP_A_RTC RCU_GPIOB
#define STEP_A_GPIO_Port GPIOB
#define STEP_A_Pin GPIO_PIN_2

#define STEP_B_RTC RCU_GPIOB
#define STEP_B_GPIO_Port GPIOB
#define STEP_B_Pin GPIO_PIN_3

#define STEP_C_RTC RCU_GPIOB
#define STEP_C_GPIO_Port GPIOB
#define STEP_C_Pin GPIO_PIN_4

#define STEP_D_RTC RCU_GPIOB
#define STEP_D_GPIO_Port GPIOB
#define STEP_D_Pin GPIO_PIN_5

#define STEPA(X)                                                               \
  X ? (gpio_bit_write(STEP_A_GPIO_Port, STEP_A_Pin, SET))                      \
    : (gpio_bit_write(STEP_A_GPIO_Port, STEP_A_Pin, RESET))
#define STEPB(X)                                                               \
  X ? (gpio_bit_write(STEP_B_GPIO_Port, STEP_B_Pin, SET))                      \
    : (gpio_bit_write(STEP_B_GPIO_Port, STEP_B_Pin, RESET))
#define STEPC(X)                                                               \
  X ? (gpio_bit_write(STEP_C_GPIO_Port, STEP_C_Pin, SET))                      \
    : (gpio_bit_write(STEP_C_GPIO_Port, STEP_C_Pin, RESET))
#define STEPD(X)                                                               \
  X ? (gpio_bit_write(STEP_D_GPIO_Port, STEP_D_Pin, SET))                      \
    : (gpio_bit_write(STEP_D_GPIO_Port, STEP_D_Pin, RESET))


typedef struct step_motor_i {
  void (*init)(void);
  void (*stop)(void);
  void (*corotation)(uint16_t);
  void (*reverse)(uint16_t);
	void (*set_speed)(uint8_t);
} step_motor_i;

extern step_motor_i step_motor;

#endif
