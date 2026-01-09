#ifndef __iic_H
#define __iic_H

#include "gd32f4xx.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  rcu_periph_enum scl_rtc;
  uint32_t scl_port;
  uint16_t scl_pin;
  rcu_periph_enum sda_rtc;
  uint32_t sda_port;
  uint16_t sda_pin;
} IIC_Config;

void IIC_Init(IIC_Config *cfg);

void SDA_IN(IIC_Config *cfg);
void SDA_OUT(IIC_Config *cfg);

void IIC_Start(IIC_Config *cfg);
void IIC_Stop(IIC_Config *cfg);

uint8_t IIC_Wait_Ack(IIC_Config *cfg);
void IIC_Ack(IIC_Config *cfg);
void IIC_NAck(IIC_Config *cfg);

void IIC_Send_Byte(IIC_Config *cfg, uint8_t txd);
uint8_t IIC_Read_Byte(IIC_Config *cfg, unsigned char ack);

// 7-bit DevAddress 的通用接口（保留）
void IIC_Master_Transmit(IIC_Config *cfg, uint16_t DevAddress, uint8_t *pData,
                         uint16_t Size);
uint8_t IIC_Master_Receive(IIC_Config *cfg, uint16_t DevAddress, uint8_t *pData,
                           uint16_t Size);

// 新增：总线恢复（SDA 卡低时救回来）
void IIC_BusRecover(IIC_Config *cfg);

#ifdef __cplusplus
}
#endif

#endif
