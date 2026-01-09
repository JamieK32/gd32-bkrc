#ifndef __ultrasonic_H_
#define __ultrasonic_H_

#include "gd32f4xx.h"
#include "string.h"
#include "stdio.h"
#include "delay.h"
#include "timer.h"
#include "exti.h"
#include "oled.h"


#define Ultrasonic_OUT_RTC 			RCU_GPIOA
#define Ultrasonic_OUT_GPIO_Port	GPIOA
#define Ultrasonic_OUT_Pin 			GPIO_PIN_12

#define Ultrasonic_IN_RTC 			RCU_GPIOA
#define Ultrasonic_IN_GPIO_Port		GPIOA
#define Ultrasonic_IN_Pin 			GPIO_PIN_11

#define EXTI_x_IRQn 				EXTI10_15_IRQn
#define	EXTI_SOURCE_GPIOx			EXTI_SOURCE_GPIOA
#define EXTI_SOURCE_PINx			EXTI_SOURCE_PIN12                   
#define EXTI_x  					EXTI_12
#define EXTI_IRQHandler 			EXTI10_15_IRQHandler

extern volatile uint32_t	status;		// 计数值
extern volatile uint32_t	real_time;	// 读回值
extern volatile float 	dis_temp;	// 距离计算值

typedef struct ultrasonic_t {
	void (*init)(void);
	void (*test)(void);
	float (*get_cm)(void);
} ultrasonic_t;

extern ultrasonic_t ultrasonic;

void Ultrasonic_Init(void);
void Ultrasonic_Test(void);
float Ultrasonic_Get_Cm(void);

#endif
