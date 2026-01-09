#ifndef __74HC595_H
#define __74HC595_H

#include "gd32f4xx.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "delay.h"
#include "matrix_data.h"

#define OE_RTC 		RCU_GPIOC
#define OE_GPIO_Port 	GPIOC
#define OE_Pin 		GPIO_PIN_0

#define RCLK_RTC 		RCU_GPIOC
#define RCLK_GPIO_Port 	GPIOC
#define RCLK_Pin 		GPIO_PIN_2

#define SCLK_RTC 		RCU_GPIOC
#define SCLK_GPIO_Port 	GPIOC
#define SCLK_Pin 		GPIO_PIN_1

#define SER_RTC 	  	RCU_GPIOC
#define SER_GPIO_Port	GPIOC
#define SER_Pin 		GPIO_PIN_3

#define RCLK_H gpio_bit_set(RCLK_GPIO_Port,RCLK_Pin)
#define RCLK_L gpio_bit_reset(RCLK_GPIO_Port,RCLK_Pin)

#define SCLK_H gpio_bit_set(SCLK_GPIO_Port,SCLK_Pin)
#define SCLK_L gpio_bit_reset(SCLK_GPIO_Port,SCLK_Pin)

#define SER_H  gpio_bit_set(SER_GPIO_Port,SER_Pin)
#define SER_L  gpio_bit_reset(SER_GPIO_Port,SER_Pin)

typedef struct matrix {
	uint16_t img_data_index;
	void (*init)(void);
	void (*display_static)(uint8_t *data);
	void (*display_scroll)(uint8_t *data, uint16_t len, uint8_t direction); 
	void (*display_countup_9)(void);
	void (*display_countdown_9)(void);
	void (*display_countup_99)(void);
	void (*display_countdown_99)(void);
	void (*clear)(void);
	void (*display_img)(uint8_t *);
} matrix_t;

extern matrix_t matrix;


void HC595_Init(void);
void HC595_Send_16Bit(uint16_t data);
void Matrix_Display_Static(uint8_t* data);
void Matrix_Display_Scroll(uint8_t *data, uint16_t len, uint8_t direction);
void Matrix_Display_CountUp_Blocking_9(void);
void Matrix_Display_Count_Down_Blocking_9(void);
void Matrix_Display_CountUp_99_Blocking(void);
void Matrix_Display_CountDown_99_Blocking(void);
void Matrix_Display_Clear(void);
void Matrix_Display_Img(uint8_t *data);

#endif 


