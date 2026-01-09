#ifndef __SMG_H_
#define __SMG_H_
#include "gd32f4xx.h"
#include "delay.h"

#define SMG_SEG1_RTC 		RCU_GPIOA
#define SMG_SEG1_GPIO_Port	GPIOA
#define SMG_SEG1_Pin 		GPIO_PIN_8

#define SMG_SEG2_RTC 		RCU_GPIOC
#define SMG_SEG2_GPIO_Port	GPIOC
#define SMG_SEG2_Pin 		GPIO_PIN_9

#define LED595_RCK_RTC 		RCU_GPIOC
#define LED595_RCK_GPIO_Port	GPIOC
#define LED595_RCK_Pin 		GPIO_PIN_0

#define LED595_SCK_RTC 		RCU_GPIOC
#define LED595_SCK_GPIO_Port	GPIOC
#define LED595_SCK_Pin 		GPIO_PIN_1

#define LED595_SOE_RTC 		RCU_GPIOC
#define LED595_SOE_GPIO_Port	GPIOC
#define LED595_SOE_Pin 		GPIO_PIN_2

#define LED595_DATA_RTC 		RCU_GPIOC
#define LED595_DATA_GPIO_Port	GPIOC
#define LED595_DATA_Pin 		GPIO_PIN_3


//74HC595口线高低电平宏定义

#define    LED595_DATA_SET      gpio_bit_write(LED595_DATA_GPIO_Port, LED595_DATA_Pin, SET);
#define    LED595_DATA_RESET    gpio_bit_write(LED595_DATA_GPIO_Port, LED595_DATA_Pin, RESET);

#define    LED595_RCK_SET       gpio_bit_write(LED595_RCK_GPIO_Port, LED595_RCK_Pin, SET);
#define    LED595_RCK_RESET     gpio_bit_write(LED595_RCK_GPIO_Port, LED595_RCK_Pin, RESET);

#define    LED595_SCK_SET     gpio_bit_write(LED595_SCK_GPIO_Port, LED595_SCK_Pin, SET);
#define    LED595_SCK_RESET   gpio_bit_write(LED595_SCK_GPIO_Port, LED595_SCK_Pin, RESET);

#define	   SOE_SET				gpio_bit_write(LED595_SOE_GPIO_Port, LED595_SOE_Pin, SET);						//高电平
#define	   SOE_RESET			gpio_bit_write(LED595_SOE_GPIO_Port, LED595_SOE_Pin, RESET);					//低电平

#define	   SEG1_SET				gpio_bit_write(SMG_SEG1_GPIO_Port, SMG_SEG1_Pin, SET);						//高电平
#define	   SEG1_RESET			gpio_bit_write(SMG_SEG1_GPIO_Port, SMG_SEG1_Pin, RESET);					//低电平

#define	   SEG2_SET				gpio_bit_write(SMG_SEG2_GPIO_Port, SMG_SEG2_Pin, SET);  					//高电平
#define	   SEG2_RESET			gpio_bit_write(SMG_SEG2_GPIO_Port, SMG_SEG2_Pin, RESET);					//低电平


void LED595Init(void);
void LED595SendData(unsigned char OutData);
void Nixie_ShowHexNum(uint8_t num);
void Nixie_ShowNum(uint8_t num);
void Nixie_Count_Down_Blocking(uint8_t time);
void Nixie_Count_Up_Blocking(void);
void Nixie_Off(void);

typedef struct nixie_tube_t {
	void (*init)(void);
	void (*off)(void);
	void (*show_num)(uint8_t num);
	void (*show_count_down_blocking)(uint8_t);
	void (*show_count_up_blocking)(void);
} nixie_tube_t;

extern nixie_tube_t nixie_tube;

#endif

