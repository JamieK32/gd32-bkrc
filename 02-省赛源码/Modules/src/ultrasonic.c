#include "ultrasonic.h"

//连接开发板P22端口

ultrasonic_t ultrasonic = {
	.init = Ultrasonic_Init,
	.test = Ultrasonic_Test,
	.get_cm = Ultrasonic_Get_Cm,
};

volatile uint32_t real_time 	= 0;	// 读回值
volatile float dis_temp 		= 0;	// 距离计算值
float show_distance = 0;
volatile uint32_t status 	= 0;	// 计数值

void Ultrasonic_Hardware_Init(void)
{
	rcu_periph_clock_enable(Ultrasonic_IN_RTC);
	gpio_mode_set(Ultrasonic_IN_GPIO_Port, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLDOWN, Ultrasonic_IN_Pin);
	gpio_output_options_set(Ultrasonic_IN_GPIO_Port, GPIO_OTYPE_PP, GPIO_OSPEED_200MHZ, Ultrasonic_IN_Pin);
}

void Ultrasonic_Software_Init(void)
{
	TIMER1_init(10-1,168-1);	//1Mhz的计数频率，计数到10为10us  
	my_EXTI_Init();
}

void Ultrasonic_Init(void)
{
	Ultrasonic_Hardware_Init();
	Ultrasonic_Software_Init();
}

// 提取公共部分，发送超声波信号
void Ultrasonic_Send_Pulse(void) {
    gpio_bit_set(Ultrasonic_IN_GPIO_Port, Ultrasonic_IN_Pin);
    delay_us(10);    
    gpio_bit_reset(Ultrasonic_IN_GPIO_Port, Ultrasonic_IN_Pin);
    delay_us(10);        
}

// 计算并返回距离值
float Ultrasonic_Get_Cm(void) {
    static float output = 0;
    static uint32_t show_time = 0;
    static float show_distance = 0;
    
    timer_enable(TIMER1);  // 启动定时器1
    status = 0;  // 重置状态
    
    Ultrasonic_Send_Pulse();  // 发送超声波脉冲
    
    delay_ms(20);  // 延时等待超声波反射时间
    
    show_time++;  // 增加计时器
    show_distance += dis_temp;  // 累加距离
    
    if(show_time == 10) {  // 每10次显示平均距离
        output = show_distance / 100.0f;
        show_time = 0;
        show_distance = 0;
    }
    
    return output / 1.209f;
}

// 测试函数显示距离
void Ultrasonic_Test(void) {
    static uint32_t show_time = 0;
    static float show_distance = 0;
    
    timer_enable(TIMER1);  // 启动定时器1
    status = 0;  // 重置状态
    
    Ultrasonic_Send_Pulse();  // 发送超声波脉冲
    
    delay_ms(20);  // 延时等待超声波反射时间
    
    show_time++;  // 增加计时器
    show_distance += dis_temp;  // 累加距离
    
    if(show_time == 10) {  // 每10次显示平均距离
        oled.print(10 * 1, 4, 16, "%5.1f CM    ", show_distance / 100.0f);
        show_time = 0;
        show_distance = 0;
    }
}

