#ifndef SPEAK_SPEAK_H_
#define SPEAK_SPEAK_H_

#include "gd32f4xx.h"
#include "gd32f4xx_it.h"
#include "led.h"
#include "oled.h"
#include "delay.h"
#include "usart.h"

extern volatile uint8_t voiceFlag;

typedef struct bkrc_speak_i 
{
	void (*init)(void);
	void (*asr)(void);
	void (*callback)(uint8_t);
	void (*send_cmd)(uint8_t);
	void (*announcement_digit)(int);
} bkrc_speak_i;

static void Voice_Init(void);
static void Voice_Drive(void);
static void VoiceAnnouncementDigit(int digit);
static void Bkrc_Send_Cmd(uint8_t cmd);

extern bkrc_speak_i bkrc_speak;

#endif /* SPEAK_SPEAK_H_ */
