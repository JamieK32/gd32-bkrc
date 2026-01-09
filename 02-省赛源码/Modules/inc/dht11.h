#ifndef __DHT11_H
#define __DHT11_H

#include "gd32f4xx.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "delay.h"
#include "oled.h"

#define DQ_RTC 			RCU_GPIOC
#define DQ_GPIO_Port	GPIOC
#define DQ_Pin 			GPIO_PIN_7

//IO方向设置
#define DHT11_IO_IN()    DQ_IN()
#define DHT11_IO_OUT()   DQ_OUT()

////IO操作函数			  					    			   
#define	DHT11_DQ_OUT_1 	gpio_bit_write(DQ_GPIO_Port,DQ_Pin,SET)	
#define	DHT11_DQ_OUT_0 	gpio_bit_write(DQ_GPIO_Port,DQ_Pin,RESET)
#define	DHT11_DQ_IN()  	gpio_input_bit_get(DQ_GPIO_Port,DQ_Pin)

typedef struct dht11_i {
	uint16_t (*get_temperature)(void);
	uint16_t (*get_humidity)(void);
	void (*init)(void);
	void (*stop)(void);
	void (*test)(void);
} dht11_i;


void DHT11_Update(void);
uint8_t DHT11_Read_Data(uint16_t *temp,uint16_t *humi);	//读取温湿度
uint8_t DHT11_Read_Byte(void);				//读出一个字节
uint8_t DHT11_Read_Bit(void);				//读出一个位
uint8_t DHT11_Check(void);					//检测是否存在DHT11
void DHT11_Rst(void);						//复位DHT11   
void DQ_OUT(void);
void DQ_IN(void);
void DHT11_WHILE(void);
void DHT11_Init(void);
void DHT11_Test(void);

extern dht11_i dht11;

#endif 
