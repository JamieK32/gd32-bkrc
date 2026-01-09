#ifndef __IR_H
#define __IR_H

#include "gd32f4xx.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "delay.h"
#include "oled.h"

#define IR_RTC 			RCU_GPIOC
#define IR_GPIO_Port	GPIOC
#define IR_Pin 			GPIO_PIN_8

#define	IR	gpio_input_bit_get(IR_GPIO_Port,IR_Pin) 

void IR_Init(void);
void IR_Test(void);
uint8_t IR_Get_State(void);

typedef struct ir_t {
	void (*init)(void);
	uint8_t (*get_state)(void);
	void (*test)(void);
} ir_t;

extern ir_t ir;



#endif 

