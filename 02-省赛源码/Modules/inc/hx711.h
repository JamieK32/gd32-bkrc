#ifndef __HX711_H
#define __HX711_H

#include "gd32f4xx.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "gd32f4xx_i2c.h"
#include "delay.h"
#include "oled.h"

#define HX711_SCK_RTC RCU_GPIOC
#define HX711_SCK_PORT GPIOC
#define HX711_SCK_PIN GPIO_PIN_9

#define HX711_DOUT_RTC RCU_GPIOA
#define HX711_DOUT_PORT GPIOA
#define HX711_DOUT_PIN GPIO_PIN_8

#define HX711_SCK_LOW (GPIO_BC(HX711_SCK_PORT) |= HX711_SCK_PIN)
#define HX711_SCK_HIGH (GPIO_BOP(HX711_SCK_PORT) |= HX711_SCK_PIN)
#define HX711_SCK_IN (GPIO_ISTAT(HX711_SCK_PORT) & HX711_SCK_PIN)
#define HX711_DOUT_IN (GPIO_ISTAT(HX711_DOUT_PORT) & HX711_DOUT_PIN)

typedef struct hx711_i
{
	void (*init)(void);
	long (*get_weight)(void);
	void (*test)(void);
} hx711_i;

extern hx711_i hx711;

#endif
