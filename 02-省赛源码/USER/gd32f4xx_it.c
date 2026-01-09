/*!
    \file    gd32f4xx_it.c
    \brief   interrupt service routines

    \version 2016-08-15, V1.0.0, firmware for GD32F4xx
    \version 2018-12-12, V2.0.0, firmware for GD32F4xx
    \version 2020-09-30, V2.1.0, firmware for GD32F4xx
*/

/*
    Copyright (c) 2020, GigaDevice Semiconductor Inc.

    Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright notice,
       this list of conditions and the following disclaimer in the documentation
       and/or other materials provided with the distribution.
    3. Neither the name of the copyright holder nor the names of its
contributors may be used to endorse or promote products derived from this
software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "gd32f4xx_it.h"
#include "c1016.h"
#include "main.h"
#include "speak.h"
#include "delay.h"
#include "timer.h"
#include "usart.h"
#include "ultrasonic.h"
#include "button_callback.h"
#include "queue.h"
#include "motor.h"

#define LCD_PACKET_HEADER 0xAA
#define LCD_PACKET_FOOTER 0x04
#define COMMAND_CODE 0xFE
#define DATA_CODE 0xFF
#define USART5_DATA_SIZE 26

uint8_t usart1_rx_data;
uint8_t usart1_rxbuffer[200];
uint16_t usart1_rx_size = 200;
uint16_t usart1_rxcount = 0;
uint16_t USART1_FLAG = 0x00;

volatile uint8_t ScreenDataByte = 0x00;
uint8_t lcd_rx_data[256];
uint16_t lcd_data_size = 0;
static UartData uartData;

uint8_t usart2_rx_data;
uint8_t usart2_rxbuffer[200];
uint16_t usart2_rx_size = 200;
uint16_t usart2_rxcount = 0;
uint16_t USART2_FLAG = 0x00;



uint8_t raspi_rx_data[256];
uint16_t raspi_data_size = 0;

/*!
    \brief    this function handles NMI exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void NMI_Handler(void) {}

/*!
    \brief    this function handles HardFault exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void HardFault_Handler(void) {
  /* if Hard Fault exception occurs, go to infinite loop */
  while (1) {
  }
}

/*!
    \brief    this function handles MemManage exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void MemManage_Handler(void) {
  /* if Memory Manage exception occurs, go to infinite loop */
  while (1) {
  }
}

/*!
    \brief    this function handles BusFault exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void BusFault_Handler(void) {
  /* if Bus Fault exception occurs, go to infinite loop */
  while (1) {
  }
}

/*!
    \brief    this function handles UsageFault exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void UsageFault_Handler(void) {
  /* if Usage Fault exception occurs, go to infinite loop */
  while (1) {
  }
}

/*!
    \brief    this function handles SVC exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
// void SVC_Handler(void)
//{
// }

/*!
    \brief    this function handles DebugMon exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void DebugMon_Handler(void) {}

/*!
    \brief    this function handles PendSV exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
// void PendSV_Handler(void)
//{
// }

/*!
    \brief    this function handles SysTick exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
// void SysTick_Handler(void)
//{
//
// }

void EXTI2_IRQHandler(void) {
    if (exti_interrupt_flag_get(EXTI_2) != RESET) {
        if (gpio_input_bit_get(ENCODER_GPIO_PORT, ENCODER_H1_PIN)) {
            if (gpio_input_bit_get(ENCODER_GPIO_PORT, ENCODER_H2_PIN)) {
                encoder_count--;
            } else {
                encoder_count++;
            }
        } else {
            if (gpio_input_bit_get(ENCODER_GPIO_PORT, ENCODER_H2_PIN)) {
                encoder_count++;
            } else {
                encoder_count--;
            }
        }
        exti_interrupt_flag_clear(EXTI_2);
    }
}

void EXTI3_IRQHandler(void) {
    if (exti_interrupt_flag_get(EXTI_3) != RESET) {
        if (gpio_input_bit_get(ENCODER_GPIO_PORT, ENCODER_H2_PIN)) {
            if (gpio_input_bit_get(ENCODER_GPIO_PORT, ENCODER_H1_PIN)) {
                encoder_count++;
            } else {
                encoder_count--;
            }
        } else {
            if (gpio_input_bit_get(ENCODER_GPIO_PORT, ENCODER_H1_PIN)) {
                encoder_count--;
            } else {
                encoder_count++;
            }
        }
        exti_interrupt_flag_clear(EXTI_3);
    }
}



//用于超声波计时
void TIMER1_IRQHandler(void) {
  if (SET == timer_interrupt_flag_get(TIMER1, TIMER_INT_UP)) {
			status++;
  }
  /* clear TIMER interrupt flag */
  timer_interrupt_flag_clear(TIMER1, TIMER_INT_UP);
}

//系统串口中断
//空函数不能删除，不然会空指针
void USART0_IRQHandler(void) {

}

//屏幕数据解析
void process_usart2_received_data(uint8_t *data, uint16_t length) {
    if (data == NULL) {
        LOG_E("USART DATA EMPTY");
        return;
    }
    uint16_t lcd_data_ind = 0;
    if (data[0] == COMMAND_CODE) {
        ScreenDataByte = data[1];
				button_callback(ScreenDataByte);
    } else if (data[0] == DATA_CODE) {
        for (uint16_t i = 1; i < length && data[i] != '\0'; i++) {
            if (lcd_data_ind < sizeof(lcd_rx_data) - 1) {
                lcd_rx_data[lcd_data_ind++] = data[i];
            }
        }
				lcd_data_size = lcd_data_ind;
        lcd_rx_data[lcd_data_ind] = '\0';
    }
}

#if defined(NEW_XIAO_CHUANG)
//小创语音中断接受数据
void USART1_IRQHandler(void) {
  if (RESET != usart_flag_get(USART1, USART_FLAG_RBNE)) {
    usart1_rx_data = usart_data_receive(USART1);
    
    if (USART1_FLAG == 0x00) {
      // 检查起始标记
      if (usart1_rx_data == 0xF4) {
        USART1_FLAG = 0x01;
        usart1_rxcount = 0; // 重置计数器
        usart1_rxbuffer[usart1_rxcount++] = usart1_rx_data; // 保存起始标记
      }
    } else if (USART1_FLAG == 0x01) {
      // 保存数据
      usart1_rxbuffer[usart1_rxcount++] = usart1_rx_data;
      
      // 检查结束标记
      if (usart1_rx_data == 0xFB) {
        voiceFlag = 0x01; // 设置处理标志
        USART1_FLAG = 0x00; // 重置接收状态
        // usart1_rxcount 保持当前值，表示接收到的数据长度
      }
      
      // 防止缓冲区溢出
      if (usart1_rxcount >= usart1_rx_size) {
        USART1_FLAG = 0x00;
        usart1_rxcount = 0;
      }
    }
  }
}
#elif defined(OLD_XIAO_CHUANG)
void USART1_IRQHandler(void) {
  if (RESET != usart_flag_get(USART1, USART_FLAG_RBNE)) {
    usart1_rx_data = usart_data_receive(USART1);
    
    if (USART1_FLAG == 0x00) {
      // 检查第一个帧头字节 0x55
      if (usart1_rx_data == 0x55) {
        USART1_FLAG = 0x01;
        usart1_rxcount = 0;
        usart1_rxbuffer[usart1_rxcount++] = usart1_rx_data; // 保存第一个帧头字节
      }
    } else if (USART1_FLAG == 0x01) {
      // 检查第二个帧头字节 0x02
      if (usart1_rx_data == 0x02) {
        USART1_FLAG = 0x02;
        usart1_rxbuffer[usart1_rxcount++] = usart1_rx_data; // 保存第二个帧头字节
      } else {
        // 如果第二个字节不是0x02，重置接收状态
        USART1_FLAG = 0x00;
        usart1_rxcount = 0;
      }
    } else if (USART1_FLAG == 0x02) {
      // 保存数据
      usart1_rxbuffer[usart1_rxcount++] = usart1_rx_data;
      
      // 检查帧尾 0x00
      if (usart1_rx_data == 0x00) {
        voiceFlag = 0x01; // 设置处理标志，表示接收到完整数据帧
        USART1_FLAG = 0x00; // 重置接收状态，准备接收下一帧
        // usart1_rxcount 此时包含完整帧长度（包括帧头和帧尾）
      }
      
      // 防止缓冲区溢出
      if (usart1_rxcount >= usart1_rx_size) {
        USART1_FLAG = 0x00;
        usart1_rxcount = 0;
      }
    }
  }
}
#endif


//串口屏数据接收
void USART2_IRQHandler(void) {
  if (RESET != usart_interrupt_flag_get(USART2, USART_INT_FLAG_RBNE)) {
    uint8_t receivedData = usart_data_receive(USART2);
    if (receivedData == LCD_PACKET_HEADER) {
      USART2_FLAG = 0x01;
      usart2_rxcount = 0;
    } else if (receivedData == LCD_PACKET_FOOTER && USART2_FLAG == 0x01) {
      USART2_FLAG = 0x00;
			memcpy(uartData.data, usart2_rxbuffer, usart2_rxcount);
			uartData.size = usart2_rxcount;
      BaseType_t xHigherPriorityTaskWoken = pdFALSE;
			xQueueSendFromISR(uartQueue, &uartData, &xHigherPriorityTaskWoken);
      memset(usart2_rxbuffer, 0, sizeof(usart2_rxbuffer)); // clear the buffer
    } else if (USART2_FLAG == 0x01) {
      if (usart2_rxcount < usart2_rx_size) {
        usart2_rxbuffer[usart2_rxcount++] = receivedData;
      }
    }
  }
}

//c1016串口接收
void USART5_IRQHandler(void) {
    static uint8_t data_index = 0;
    static uint8_t max_size = 26;
    static bool start_receive_flag = false;

    // 循环直到接收到26个字节
    // 确保RBNE标志已经准备好数据
    if (RESET != usart_flag_get(USART5, USART_FLAG_RBNE)) {
        uint8_t byte = usart_data_receive(USART5);

        // 打印接收到的字节，用于调试
        // printf("Received byte: %02X\n", byte); // 可选的调试输出

        // 当接收到0xAA时，标记为开始接收
        if (byte == 0xAA && !start_receive_flag) {
            start_receive_flag = true;  // 启动接收
        }

        // 如果开始接收并且接收到数据
        if (start_receive_flag) {
            c1016_rx_buffer[data_index++] = byte;

            // 如果接收到的数据已达到最大长度，则停止接收
            if (data_index == max_size) {
                start_receive_flag = false;  // 重置接收标志
                data_index = 0;              // 重置索引
            }
        }
    }
}



// 定时器中断处理函数 弃用
//void TIMER3_IRQHandler(void) {
//    if (timer_interrupt_flag_get(TIMER3, TIMER_INT_UP) == SET) {
//        global_count++; // 计数器递增
//        timer_interrupt_flag_clear(TIMER3, TIMER_INT_UP);
//    }
//}
