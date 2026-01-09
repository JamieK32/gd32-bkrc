#include "speak.h"
#include "main.h"

//P14端口 此模块25省赛已弃用

// 全局变量
volatile uint8_t voiceFlag = 0;

// USART 配置
USART_Config speakUsartConfig;

bkrc_speak_i bkrc_speak = {
    .init = Voice_Init,
    .asr = Voice_Drive,
		.send_cmd = Bkrc_Send_Cmd,
		.announcement_digit = VoiceAnnouncementDigit,
};

// 初始化语音模块
static void Voice_Init(void) { 
	speakUsartConfig.af_x = GPIO_AF_7;
	#if defined(NEW_XIAO_CHUANG)
	speakUsartConfig.baud = 9600;
	#elif defined(OLD_XIAO_CHUANG)
	speakUsartConfig.baud = 115200;
	#endif
	speakUsartConfig.nvic_irq = USART1_IRQn;
	speakUsartConfig.rx_pin = GPIO_PIN_3;
	speakUsartConfig.rx_port = GPIOA;
	speakUsartConfig.rx_rtc = RCU_GPIOA;
	speakUsartConfig.tx_pin = GPIO_PIN_2;
	speakUsartConfig.tx_port = GPIOA;
	speakUsartConfig.tx_rtc = RCU_GPIOA;
	speakUsartConfig.usartx_rtc = RCU_USART1;
	speakUsartConfig.usart_x = USART1;
	USART_Init(&speakUsartConfig);
}

// 判断接收的数据部分并返回状态值
static void Voice_Drive(void) {
  if (voiceFlag == 0x01) {
		bkrc_speak.callback(usart1_rxbuffer[2]);
    voiceFlag = 0x00;
  }
}

static void Bkrc_Send_Cmd(uint8_t cmd) {
	#if defined(NEW_XIAO_CHUANG)
	uint8_t Buffer[4] = {0xf4, 0xf5, cmd, 0xfb};
	USART_Send_Buff(&speakUsartConfig, Buffer, 4);
	#elif defined(OLD_XIAO_CHUANG)
	uint8_t Buffer[1] = {cmd};
	USART_Send_Buff(&speakUsartConfig, Buffer, 1);
	#endif
}

static void VoiceAnnouncementDigit(int digit) {
  uint16_t time = 270;
  
  //共用相同的首值和映射表
	#if defined(OLD_XIAO_CHUANG)
  uint8_t first = 0x00;
	#elif defined(NEW_XIAO_CHUANG)
	uint8_t first = 0x46;
	#endif
  uint16_t map[30][2] = {{0, first},        {1, first + 1},   {2, first + 2},
                         {3, first + 3},    {4, first + 4},   {5, first + 5},
                         {6, first + 6},    {7, first + 7},   {8, first + 8},
                         {9, first + 9},    {10, first + 10}, {100, first + 11},
                         {1000, first + 12}};
  
  #if defined(NEW_XIAO_CHUANG)
  uint8_t Buffer[4] = {0xf4, 0xf5, 0x00, 0xfb};
  #elif defined(OLD_XIAO_CHUANG)
  uint8_t Buffer[1] = {0};
  #endif

  if (digit == 0) {
    #if defined(NEW_XIAO_CHUANG)
    Buffer[2] = first;
    USART_Send_Buff(&speakUsartConfig, Buffer, 4);
    #elif defined(OLD_XIAO_CHUANG)
    Buffer[0] = first;
    USART_Send_Buff(&speakUsartConfig, Buffer, 1);
    #endif
    return;
  }

  if (digit >= 1000) {
    int thousands = digit / 1000;
    #if defined(NEW_XIAO_CHUANG)
    Buffer[2] = map[thousands][1];
    USART_Send_Buff(&speakUsartConfig, Buffer, 4);
    delay_ms(time);
    Buffer[2] = map[12][1]; // 播报 "千"
    USART_Send_Buff(&speakUsartConfig, Buffer, 4);
    #elif defined(OLD_XIAO_CHUANG)
    Buffer[0] = map[thousands][1];
    USART_Send_Buff(&speakUsartConfig, Buffer, 1);
    delay_ms(time);
    Buffer[0] = map[12][1]; // 播报 "千"
    USART_Send_Buff(&speakUsartConfig, Buffer, 1);
    #endif
    delay_ms(time);
    digit %= 1000;
  }

  if (digit >= 100) {
    int hundreds = digit / 100;
    #if defined(NEW_XIAO_CHUANG)
    Buffer[2] = map[hundreds][1];
    USART_Send_Buff(&speakUsartConfig, Buffer, 4);
    delay_ms(time);
    Buffer[2] = map[11][1]; // 播报 "百"
    USART_Send_Buff(&speakUsartConfig, Buffer, 4);
    #elif defined(OLD_XIAO_CHUANG)
    Buffer[0] = map[hundreds][1];
    USART_Send_Buff(&speakUsartConfig, Buffer, 1);
    delay_ms(time);
    Buffer[0] = map[11][1]; // 播报 "百"
    USART_Send_Buff(&speakUsartConfig, Buffer, 1);
    #endif
    delay_ms(time);
    digit %= 100;
  }

  if (digit >= 10) {
    int tens = digit / 10;
    #if defined(NEW_XIAO_CHUANG)
    Buffer[2] = map[tens][1];
    USART_Send_Buff(&speakUsartConfig, Buffer, 4);
    delay_ms(time);
    Buffer[2] = map[10][1]; // 播报 "十"
    USART_Send_Buff(&speakUsartConfig, Buffer, 4);
    #elif defined(OLD_XIAO_CHUANG)
    Buffer[0] = map[tens][1];
    USART_Send_Buff(&speakUsartConfig, Buffer, 1);
    delay_ms(time);
    Buffer[0] = map[10][1]; // 播报 "十"
    USART_Send_Buff(&speakUsartConfig, Buffer, 1);
    #endif
    delay_ms(time);
    digit %= 10;
  }

  if (digit > 0) {
    #if defined(NEW_XIAO_CHUANG)
    Buffer[2] = map[digit][1];
    USART_Send_Buff(&speakUsartConfig, Buffer, 4);
    #elif defined(OLD_XIAO_CHUANG)
    Buffer[0] = map[digit][1];
    USART_Send_Buff(&speakUsartConfig, Buffer, 1);
    #endif
    delay_ms(time);
  }
}
