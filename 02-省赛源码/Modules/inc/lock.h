#ifndef _LOCK_H_
#define _LOCK_H_


#include "oled.h"
#include "delay.h"
#include "gd32f4xx.h"

#define LOCK_RTC 		RCU_GPIOD
#define LOCK_GPIO_Port	GPIOD
#define LOCK_Pin 		GPIO_PIN_15

typedef struct lock_i 
{
	uint8_t state;
	void (*init)(void);
	void (*control)(unsigned char);
} lock_i;

void Lock_Init(void);
void Lock_Control(unsigned char level);

extern lock_i lock;

#endif

