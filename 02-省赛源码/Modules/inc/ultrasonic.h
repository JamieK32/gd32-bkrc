#ifndef ULTRASONIC_H
#define ULTRASONIC_H

#include <stdint.h>
#include "gd32f4xx.h"
#include "string.h"
#include "stdio.h"
#include "delay.h"
#include "timer.h"
#include "oled.h"

#define ULTRASONIC_PORT_NAME "P22"

#ifdef __cplusplus
extern "C" {
#endif

void Ultrasonic_Init(void);

/**
 * 初始化后反复调用即可：
 * - 内部自动持续测距（不需要你先 Trigger）
 * - 返回最近一次的滤波距离（单位 cm）
 */
float Ultrasonic_Get_Cm(void);

/**
 * 可选：测试显示（内部也是自动持续测距）
 */
void Ultrasonic_Test(void);

/**
 * EXTI中断里调用
 */
void Ultrasonic_EXTI_Handler(void);

#ifdef __cplusplus
}
#endif
#endif
