#ifndef __MATRIX_H
#define __MATRIX_H

#include "gd32f4xx.h"
#include "string.h"
#include "delay.h"
#include "matrix_data.h"   // 你原来的字库：matrix_nums_data / matrix_data_2 等

#define MATRAIX_PORT_NAME "P25"

// ======= 74HC595 引脚定义（保留你原来的） =======
#define OE_RTC          RCU_GPIOC
#define OE_GPIO_Port    GPIOC
#define OE_Pin          GPIO_PIN_0

#define RCLK_RTC        RCU_GPIOC
#define RCLK_GPIO_Port  GPIOC
#define RCLK_Pin        GPIO_PIN_2

#define SCLK_RTC        RCU_GPIOC
#define SCLK_GPIO_Port  GPIOC
#define SCLK_Pin        GPIO_PIN_1

#define SER_RTC         RCU_GPIOC
#define SER_GPIO_Port   GPIOC
#define SER_Pin         GPIO_PIN_3

#define RCLK_H gpio_bit_set(RCLK_GPIO_Port,RCLK_Pin)
#define RCLK_L gpio_bit_reset(RCLK_GPIO_Port,RCLK_Pin)

#define SCLK_H gpio_bit_set(SCLK_GPIO_Port,SCLK_Pin)
#define SCLK_L gpio_bit_reset(SCLK_GPIO_Port,SCLK_Pin)

#define SER_H  gpio_bit_set(SER_GPIO_Port,SER_Pin)
#define SER_L  gpio_bit_reset(SER_GPIO_Port,SER_Pin)

// ======= 计数方向 =======
typedef enum {
    MATRIX_COUNT_UP   = 1,
    MATRIX_COUNT_DOWN = 2,
} matrix_count_dir_t;

#define MATRIX_SCROLL_L2R   1  // 左->右
#define MATRIX_SCROLL_R2L   2  // 右->左
#define MATRIX_SCROLL_T2B   3  // 上->下
#define MATRIX_SCROLL_B2T   4  // 下->上

// loop 宏（可读性更好）
#define MATRIX_SCROLL_LOOP_ON   1
#define MATRIX_SCROLL_LOOP_OFF  0

// ======= 滚动方向（保持你原来的约定） =======
// 1: 左->右, 2: 右->左, 3: 上->下, 4: 下->上

// ======= 底层 74HC595 =======
void HC595_Init(void);
void HC595_Send_16Bit(uint16_t data);

// ======= Matrix 驱动 API =======
void Matrix_Init(void);
void Matrix_Tick(void); // 每次调用：刷新1列 + 推进状态机（建议 1ms~2ms 调一次）

// 状态 1：计时（计数）
// direction: MATRIX_COUNT_UP / MATRIX_COUNT_DOWN
// restart:  1=重新计时(重置到start), 0=继续
// start/end: 计数范围（建议 0~99）
void Matrix_TimerState(uint8_t direction, uint8_t restart, uint8_t start, uint8_t end);

// 状态 2：滚动显示（参数保持一致）
void Matrix_ScrollState(uint8_t *data, uint16_t len, uint8_t direction);

// 状态 3：显示图片（参数保持一致，默认认为 data 指向 8 字节 8列数据）
void Matrix_ImgState(uint8_t *data);

// clear 保留
void Matrix_Clear(void);

// 扩展：滚动显示（增加 loop 与 speed_ms）
// loop: 1=循环 0=不循环
// speed_ms: 每步滚动间隔(ms)，例如 20/30/50/100
void Matrix_ScrollStateEx(uint8_t *data, uint16_t len, uint8_t direction,
                          uint8_t loop, uint16_t speed_ms);

#endif
