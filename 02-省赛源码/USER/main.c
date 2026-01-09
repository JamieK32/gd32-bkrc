#include "gd32f4xx.h"
#include "main.h"
#include "syslib.h"

//系统串口用来调试信息
//常用调试宏 LOG_I(调试信息) LOG_W(警告信息) LOG_E(错误信息)
USART_Config systemUsartConfig = {
    .tx_rtc = RCU_GPIOA,
    .rx_rtc = RCU_GPIOA,
    .usartx_rtc = RCU_USART0,
    .tx_port = GPIOA,
    .tx_pin = GPIO_PIN_9,
    .rx_port = GPIOA,
    .rx_pin = GPIO_PIN_10,
    .baud = 115200,
    .usart_x = USART0,
		.af_x = GPIO_AF_7,
    .nvic_irq = USART0_IRQn,
};

//串口队列声明
QueueHandle_t uartQueue;

void UartProcessTask(void *pvParameters) {
    UartData receivedData;
    while (1) {
        // 等待队列中的数据
        if (xQueueReceive(uartQueue, &receivedData, portMAX_DELAY) == pdTRUE) {
            // 处理接收到的数据
            process_usart2_received_data(receivedData.data, receivedData.size);
        }
    }
}

void hardware_init(void) {
	systick_clksource_set(SYSTICK_CLKSOURCE_HCLK_DIV8);
	//初始化系统串口用来充当调试串口
	USART_Init(&systemUsartConfig);
	//整个系统设计基于串口屏，因此在使用系统前先会调用初始化串口屏
	//任务在串口屏接收中断中回调执行
	hmi.init();
}

void freertos_init(void) {
	uartQueue = xQueueCreate(UART_QUEUE_LENGTH, sizeof(UartData));
	if (uartQueue == NULL) {
		LOG_E("uart queue create error") ;
	}
	xTaskCreate(UartProcessTask, "UartTask", configMINIMAL_STACK_SIZE * 3, NULL, tskIDLE_PRIORITY + 3, NULL);
	vTaskStartScheduler();
}


int main(void) {
	hardware_init();
	freertos_init();
  while (1) {
		//永远也不会到这
  }
}

//重定向系统串口到printf
int fputc(int ch, FILE *f) {
  usart_data_transmit(USART0, (uint8_t)ch);
  while (RESET == usart_flag_get(USART0, USART_FLAG_TBE));
  return ch;
}
