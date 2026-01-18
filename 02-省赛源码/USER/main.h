/*!
    \file    main.h
    \brief   the header file of main

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

#ifndef __MAIN_H
#define __MAIN_H

#include <stdio.h>
#include "stdint.h"
#include "freertos.h"
#include "task.h"
#include "queue.h"

#define UART_QUEUE_LENGTH 10
#define UART_DATA_LEN 256

typedef struct {
    uint8_t data[UART_DATA_LEN];
    uint16_t size;
} UartData;

extern QueueHandle_t uartQueue;

// 打印调试信息
#define LOG_I(fmt, ...) printf("[INFO] " fmt "\n", ##__VA_ARGS__)

// 打印错误信息
#define LOG_E(fmt, ...) printf("[ERROR] " fmt "\n", ##__VA_ARGS__)

// 打印警告信息
#define LOG_W(fmt, ...) printf("[Waring] " fmt "\n", ##__VA_ARGS__)

// 打印16进制信息
#define LOG_HEX(data, length)                                                  \
  do {                                                                         \
    for (size_t i = 0; i < length; i++) {                                      \
      printf("%02X ", (unsigned char)(data[i]));                               \
    }                                                                          \
    printf("\n");                                                              \
  } while (0)

#endif /* __MAIN_H */
