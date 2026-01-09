#ifndef __USART_H
#define __USART_H

#include "gd32f4xx.h"
#include "gd32f4xx_usart.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "delay.h"
#include "stdarg.h"

typedef struct USART_Config {
  rcu_periph_enum tx_rtc;
  rcu_periph_enum rx_rtc;
  rcu_periph_enum usartx_rtc;
  uint32_t tx_port;
  uint32_t tx_pin;
  uint32_t rx_port;
  uint32_t rx_pin;
  uint32_t baud;
  uint32_t af_x;
  uint32_t usart_x;
  uint8_t nvic_irq;
} USART_Config;

void USART_Init(USART_Config *config);
void USART_Send_Byte(USART_Config *config, uint8_t data);
void USART_Send_Buff(USART_Config *config, uint8_t *data, uint32_t len);
void USART_Send_Data_length(USART_Config *config, uint8_t *data, uint32_t len);
void USART_printf(USART_Config *cfg, const char *format, ...);

#endif
