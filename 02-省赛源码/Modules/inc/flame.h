#ifndef __FLAME_H
#define __FLAME_H

#include "gd32f4xx.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "delay.h"
#include "oled.h"

#define FLAME_RTC 			RCU_GPIOC
#define FLAME_GPIO_Port	GPIOC
#define FLAME_Pin 			GPIO_PIN_8

#define	FLAME	gpio_input_bit_get(FLAME_GPIO_Port,FLAME_Pin) 

typedef struct flame_i {
	void (*init)(void);
	uint8_t (*get)(void);
	void (*test)(void);
} flame_i;

extern flame_i flame;

#endif 

