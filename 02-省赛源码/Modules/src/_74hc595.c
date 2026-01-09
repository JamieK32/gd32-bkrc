#include "_74hc595.h"
#include "gd32f4xx_gpio.h"

//连接至P25端口

matrix_t matrix = {
	.img_data_index = 0,
	.init = HC595_Init,
	.display_static = Matrix_Display_Static,
	.display_scroll = Matrix_Display_Scroll,
	.display_countup_9 = Matrix_Display_CountUp_Blocking_9,
	.display_countdown_9 = Matrix_Display_Count_Down_Blocking_9,
	.display_countup_99 = Matrix_Display_CountUp_99_Blocking,
	.display_countdown_99 = Matrix_Display_CountDown_99_Blocking,
	.clear = Matrix_Display_Clear,
	.display_img = Matrix_Display_Img,
};

uint16_t col_data[8] = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};
uint16_t row_data[8] = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};

void HC595_Init(void)
{
	rcu_periph_clock_enable(RCLK_RTC);
	rcu_periph_clock_enable(SCLK_RTC);
	rcu_periph_clock_enable(SER_RTC);
	rcu_periph_clock_enable(OE_RTC);
	
	gpio_mode_set(RCLK_GPIO_Port, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLDOWN, RCLK_Pin);
	gpio_mode_set(SCLK_GPIO_Port, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLDOWN, SCLK_Pin);
	gpio_mode_set(SER_GPIO_Port, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLDOWN, SER_Pin);
	gpio_mode_set(OE_GPIO_Port, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLDOWN, OE_Pin);
	
	gpio_output_options_set(RCLK_GPIO_Port, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, RCLK_Pin);
	gpio_output_options_set(SCLK_GPIO_Port, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, SCLK_Pin);	
	gpio_output_options_set(SER_GPIO_Port, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, SER_Pin);
	gpio_output_options_set(OE_GPIO_Port, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, OE_Pin);
	
	gpio_bit_write(OE_GPIO_Port, OE_Pin, RESET);
	
	RCLK_L;
	SCLK_L;
	SER_L;
	delay_ms(500);
}


void HC595_Send_16Bit(uint16_t data) 
{  
	uint16_t i = 0;
	for(i=0;i<16;i++)           	
	{ 
	    if((data & 0x8000) == 0X8000)
		{
			SER_H;
		}
		else
		{
			SER_L;  
		}
		delay_us(10);
		SCLK_L; 
		delay_us(10);
		SCLK_H; 	              
		delay_us(10);
		data <<=1;
 	} 
	RCLK_L;         
	delay_us(10);            
	RCLK_H;
}

//共阴极点阵屏 行低电平 列高电平 有效

//用于合并数据高位是列数据，低位是行数据
uint16_t HC595_Dat_Handle(uint8_t dat,uint8_t cnt)
{
	uint16_t  Rt = 0;
	Rt = cnt;
	Rt <<= 8;  
	Rt |= (uint8_t)(~dat); 
	return Rt;
}

//列扫描每次选择一列的进行刷新显示
void Matrix_Display_Static(uint8_t* data)
{
	uint8_t i = 0;
	uint16_t dat = 0;
	for (int k = 0; k < 10; k++) {
	for(i=0;i<8;i++)
	{
		dat = HC595_Dat_Handle(*(data+i),col_data[i]);
		HC595_Send_16Bit(dat);
	}
	delay_ms(1);
	}	
}

void Matrix_Display_Scroll(uint8_t *data, uint16_t len, uint8_t direction) {
	uint8_t i = 0;
	uint16_t dat = 0;
	uint8_t frq = 30;
	for (int j = 0; j < len; j++) {
		for (int k = 0; k < frq; k++) {
			for(i=0;i<8;i++)
			{
				if (direction == 1) { //从左到右
					if (i + j < len) dat = HC595_Dat_Handle(*(data + i + j), col_data[i]);
				}
				else if (direction == 2) { //从右到左
					if (j - i >= 0) dat = HC595_Dat_Handle(*(data - i + j), col_data[i]);
				}
				else if (direction == 3) { //从上到下
					if (i + j < len) dat = HC595_Dat_Handle(row_data[i], *(data + i + j));
				}
				else if (direction == 4) { //从下到上
					if (j - i >= 0) dat = HC595_Dat_Handle(row_data[i], *(data - i + j));
				}
				HC595_Send_16Bit(dat);
				delay_ms(1);
			}
		}
	}
}


void Matrix_Display_CountUp_Blocking_9(void) {
    static uint8_t show_index = 0; // 当前显示的下标
    TickType_t prev_tick = xTaskGetTickCount(); // 获取当前系统节拍时间

    while (1) {
        // 调用显示函数（这个可能消耗一些时间）
        Matrix_Display_Static(matrix_nums_data + show_index * 8);

        // 当前时间戳
        TickType_t current_tick = xTaskGetTickCount();

        // 检测是否已经过去了 1 秒
        if ((current_tick - prev_tick) >= pdMS_TO_TICKS(1000)) {
            // 更新 prev_tick 为下一个周期点
            prev_tick = current_tick;
            // 循环增加 show_index
            show_index = (show_index + 1) % 10;
        }

        // 暂时让出 CPU 以便其他任务可以运行
        vTaskDelay(pdMS_TO_TICKS(1)); // 最小阻塞时间，避免任务持续占用 CPU
    }
}

void Matrix_Display_Count_Down_Blocking_9(void) {
    static uint8_t show_index = 9; // 当前显示的下标
    TickType_t prev_tick = xTaskGetTickCount(); // 获取当前系统节拍时间

    while (1) {
        // 调用显示函数（这个可能消耗一些时间）
        Matrix_Display_Static(matrix_nums_data + show_index * 8);
				
				// 当前时间戳
        TickType_t current_tick = xTaskGetTickCount();

        // 检测是否已经过去了 1 秒
        if ((current_tick - prev_tick) >= pdMS_TO_TICKS(1000)) {
            // 更新 prev_tick 为下一个周期点
						if (show_index == 0) show_index = 10;
            prev_tick = current_tick;
            // 循环增加 show_index
            show_index -= 1;
        }

        // 暂时让出 CPU 以便其他任务可以运行
        vTaskDelay(pdMS_TO_TICKS(1)); // 最小阻塞时间，避免任务持续占用 CPU
    }
}

void Matrix_Display_CountUp_99_Blocking(void) {
    TickType_t prev_tick = xTaskGetTickCount(); // 获取当前系统节拍时间
		uint8_t current_time = 0;
    while (1) {
        // 调用显示函数（这个可能消耗一些时间）
				uint8_t low = current_time % 10;
				uint8_t high = current_time / 10 % 10;
				char temp_buffer[8];
				memcpy(temp_buffer, matrix_data_2[high], 4);
				memcpy(temp_buffer + 4, matrix_data_2[low], 4);
        Matrix_Display_Static((uint8_t *)temp_buffer);

        // 当前时间戳
        TickType_t current_tick = xTaskGetTickCount();

        // 检测是否已经过去了 1 秒
        if ((current_tick - prev_tick) >= pdMS_TO_TICKS(1000)) {
            // 更新 prev_tick 为下一个周期点
            prev_tick = current_tick;
            // 循环增加 show_index
            current_time = (current_time + 1) % 100;
        }

        // 暂时让出 CPU 以便其他任务可以运行
        vTaskDelay(pdMS_TO_TICKS(1)); // 最小阻塞时间，避免任务持续占用 CPU
    }
}

void Matrix_Display_CountDown_99_Blocking(void) {
    TickType_t prev_tick = xTaskGetTickCount(); // 获取当前系统节拍时间
		uint8_t current_time = 99;
    while (1) {
        // 调用显示函数（这个可能消耗一些时间）
				uint8_t low = current_time % 10;
				uint8_t high = current_time / 10 % 10;
				char temp_buffer[8];
				memcpy(temp_buffer, matrix_data_2[high], 4);
				memcpy(temp_buffer + 4, matrix_data_2[low], 4);
        Matrix_Display_Static((uint8_t *)temp_buffer);

        // 当前时间戳
        TickType_t current_tick = xTaskGetTickCount();

        // 检测是否已经过去了 1 秒
        if ((current_tick - prev_tick) >= pdMS_TO_TICKS(1000)) {
            // 更新 prev_tick 为下一个周期点
            prev_tick = current_tick;
            // 循环增加 show_index
            current_time -= 1;
						if (current_time <= 0) current_time = 99;
        }

        // 暂时让出 CPU 以便其他任务可以运行
        vTaskDelay(pdMS_TO_TICKS(1)); // 最小阻塞时间，避免任务持续占用 CPU
    }
}

void Matrix_Display_Img(uint8_t *data) {
	while (1) {
		Matrix_Display_Static(data + matrix.img_data_index);
		delay_ms(1);
	}
}

void Matrix_Display_Clear(void) {
    uint8_t i;
    for (i = 0; i < 8; i++) {
        HC595_Send_16Bit(HC595_Dat_Handle(0x00, col_data[i])); // 空数据对应的行列
    }
    delay_ms(1);
}
