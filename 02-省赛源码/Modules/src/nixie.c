#include "nixie.h"
#include "main.h"
#include "stdio.h"
#include "freertos.h"
#include "task.h"

/**************************************************************************************
实验名称：数码管显示实验

硬件接线：数码管显示模块需连接“开发底板”的P25，P21端口  

实验现象：数码管上的的数字从0~9,A~F循环显示	

更新时间：2024-10-14
***************************************************************************************/

unsigned char Nixie_Tab[16] = {0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x6F,0x77,0x7C,0x39,0x5E,0x79,0x71};
unsigned char Nixie_Dot_Tab[16] = {0xBF, 0x86, 0xDB, 0xCF, 0xE6, 0xED, 0xFD, 0x87, 0xFF, 0xEF, 0xF7, 0xFC, 0xB9, 0xDE, 0xF9, 0xF1};

nixie_tube_t nixie_tube = {
	.init = LED595Init,
	.off = Nixie_Off,
	.show_num = Nixie_ShowNum,
	.show_count_down_blocking = Nixie_Count_Down_Blocking,
	.show_count_up_blocking = Nixie_Count_Up_Blocking,
};

void LED595Init(void)
{
    // Enable peripheral clocks for required GPIO ports
    rcu_periph_clock_enable(SMG_SEG1_RTC);
    rcu_periph_clock_enable(SMG_SEG2_RTC);
    rcu_periph_clock_enable(LED595_RCK_RTC);
    rcu_periph_clock_enable(LED595_SCK_RTC);
    rcu_periph_clock_enable(LED595_SOE_RTC);
    rcu_periph_clock_enable(LED595_DATA_RTC);

    // Configure GPIOs for output mode
    gpio_mode_set(SMG_SEG1_GPIO_Port, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLDOWN, SMG_SEG1_Pin);
    gpio_mode_set(SMG_SEG2_GPIO_Port, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLDOWN, SMG_SEG2_Pin);
    gpio_mode_set(LED595_RCK_GPIO_Port, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLDOWN, LED595_RCK_Pin);
    gpio_mode_set(LED595_SCK_GPIO_Port, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLDOWN, LED595_SCK_Pin);
    gpio_mode_set(LED595_SOE_GPIO_Port, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLDOWN, LED595_SOE_Pin);
    gpio_mode_set(LED595_DATA_GPIO_Port, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLDOWN, LED595_DATA_Pin);

    // Set GPIO output options (push-pull, high-speed)
    gpio_output_options_set(SMG_SEG1_GPIO_Port, GPIO_OTYPE_PP, GPIO_OSPEED_200MHZ, SMG_SEG1_Pin);
    gpio_output_options_set(SMG_SEG2_GPIO_Port, GPIO_OTYPE_PP, GPIO_OSPEED_200MHZ, SMG_SEG2_Pin);
    gpio_output_options_set(LED595_RCK_GPIO_Port, GPIO_OTYPE_PP, GPIO_OSPEED_200MHZ, LED595_RCK_Pin);
    gpio_output_options_set(LED595_SCK_GPIO_Port, GPIO_OTYPE_PP, GPIO_OSPEED_200MHZ, LED595_SCK_Pin);
    gpio_output_options_set(LED595_SOE_GPIO_Port, GPIO_OTYPE_PP, GPIO_OSPEED_200MHZ, LED595_SOE_Pin);
    gpio_output_options_set(LED595_DATA_GPIO_Port, GPIO_OTYPE_PP, GPIO_OSPEED_200MHZ, LED595_DATA_Pin);

    // Set initial states for segments and control pins
    SEG1_SET;
    SEG2_RESET;
    SOE_RESET;
}

void LED595SendData(uint8_t OutData)
{
    for (uint8_t i = 0; i < 8; i++)
    {
        // Set data bit based on the current bit of OutData
        if ((OutData << i) & 0x80)
        {
            LED595_DATA_SET;
        }
        else
        {
            LED595_DATA_RESET;
        }

        // Pulse the SCK to shift data
        LED595_SCK_RESET;
        delay_us(1);
        delay_us(1);
        LED595_SCK_SET;
    }

    // Latch the shifted data into the output register
    LED595_RCK_RESET;
    delay_us(1);
    delay_us(1);
    LED595_RCK_SET;
}


void Nixie_Select_One(uint8_t num) {
	if (num == 1) {
			SEG1_SET;
			SEG2_RESET;
			delay_ms(10);
	}	else if (num == 2) {
			SEG1_RESET;
			SEG2_SET;
	   	delay_ms(10);
	}
}

void Nixie_ShowHexNum(uint8_t num) {
	if (num < 0xFF) {
		LED595SendData(Nixie_Tab[num % 16]);
		Nixie_Select_One(1);
		LED595SendData(Nixie_Tab[num / 16]);
		Nixie_Select_One(2);
	}
	else {
		LOG_E("Data overflow");
	}
}

void Nixie_Off(void) {
	SEG2_RESET;
	SEG1_RESET;
}

void Nixie_ShowNum(uint8_t num) {
	if (num < 100) {
		LED595SendData(Nixie_Tab[num / 10]);
		Nixie_Select_One(1);
		LED595SendData(Nixie_Tab[num % 10]);
		Nixie_Select_One(2);
	} else {
		LOG_E("Data overflow");
	}
}

// 倒计时任务
void Nixie_Count_Down_Blocking(uint8_t num) {
		uint8_t time = num;
    TickType_t last_tick = xTaskGetTickCount();     // 记录上一次更新时间
    
    while (1) {
        Nixie_ShowNum(time);                        // 显示当前值
        if (time <= 0) {
            time = num;                  
        }
        
        TickType_t current_tick = xTaskGetTickCount(); // 获取当前 Tick
        if ((current_tick - last_tick) >= pdMS_TO_TICKS(1000)) { // 每 1 秒
            time--;                                    // 减少时间
            last_tick = current_tick;                  // 更新上一次时间
        }
        
        vTaskDelay(pdMS_TO_TICKS(10));              // 短暂延时，避免占用 CPU
    }
}

// 计数任务
void Nixie_Count_Up_Blocking(void) {
    uint32_t time = 0;
    TickType_t last_tick = xTaskGetTickCount();     // 记录上一次更新时间
    
    while (1) {
        Nixie_ShowNum(time);                        // 显示当前值
        TickType_t current_tick = xTaskGetTickCount(); // 获取当前 Tick
        if ((current_tick - last_tick) >= pdMS_TO_TICKS(1000)) { // 每 1 秒
            time = (time + 1) % 100;               // 增加时间，模 100
            last_tick = current_tick;                  // 更新上一次时间
        }
        
        vTaskDelay(pdMS_TO_TICKS(10));              // 短暂延时，避免占用 CPU
    }
}
