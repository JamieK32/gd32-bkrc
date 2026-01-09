#ifndef __CH455G_H
#define __CH455G_H

#include "gd32f4xx.h"
#include "math.h"
#include "oled.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "delay.h"

#define MAX_KEYS 32

// 读取按键代码命令
#define CH455_GET_KEY 0x0700 // 获取按键,返回按键代码

#define CH455_BIT_ENABLE 0x01  // 开启/关闭位
#define CH455_BIT_SLEEP 0x04   // 睡眠控制位
#define CH455_BIT_7SEG 0x08    // 7段控制位
#define CH455_BIT_INTENS1 0x10 // 1级亮度
#define CH455_BIT_INTENS2 0x20 // 2级亮度
#define CH455_BIT_INTENS3 0x30 // 3级亮度
#define CH455_BIT_INTENS4 0x40 // 4级亮度
#define CH455_BIT_INTENS5 0x50 // 5级亮度
#define CH455_BIT_INTENS6 0x60 // 6级亮度
#define CH455_BIT_INTENS7 0x70 // 7级亮度
#define CH455_BIT_INTENS8 0x00 // 8级亮度

#define CH455_SYSOFF 0x0400                            // 关闭显示、关闭键盘
#define CH455_SYSON (CH455_SYSOFF | CH455_BIT_ENABLE)  // 开启显示、键盘
#define CH455_SLEEPOFF CH455_SYSOFF                    // 关闭睡眠
#define CH455_SLEEPON (CH455_SYSOFF | CH455_BIT_SLEEP) // 开启睡眠
#define CH455_7SEG_ON (CH455_SYSON | CH455_BIT_7SEG)   // 开启七段模式
#define CH455_8SEG_ON (CH455_SYSON | 0x00)             // 开启八段模式
#define CH455_SYSON_4                                                          \
  (CH455_SYSON | CH455_BIT_INTENS4) // 开启显示、键盘、4级亮度
#define CH455_SYSON_8                                                          \
  (CH455_SYSON | CH455_BIT_INTENS8) // 开启显示、键盘、8级亮度

// CH455接口定义
#define CH455_I2C_ADDR 0x40 // CH455的地址
#define CH455_I2C_MASK 0x3E // CH455的高字节命令掩码

typedef struct keyboard_i {
  void (*init)(void);
  uint8_t (*read)(void);
  uint8_t (*get_press_count)(uint8_t);
  void (*ch455g_callback)(uint8_t);
} keyboard_i;

extern keyboard_i keyboard;

static void CH455G_Init(void);
unsigned char Read_Keyboard(void);
unsigned char Get_Key_Press_Count(unsigned char key);

#endif
