#ifndef __USART_LCD_H
#define __USART_LCD_H

#include "usart.h"


extern USART_Config lcd_usart_config;

typedef struct LcdData {
	char *name;
	char *str_data;
	uint16_t val;
} LcdData;

typedef struct hmi_i {
  void (*init)(void);
  void (*send_string)(char *, char *);
	void (*printf)(char *name, const char *, ...);
	void (*send_to_slider)(char *, int);
	void (*send_cmd)(char *);
	void (*send_to_curve)(const char *, uint8_t, uint16_t, uint16_t, uint16_t);
	uint16_t (*get_widget_val)(uint8_t *, uint16_t, const char *);
	void (*get_wigit_string)(uint8_t *, uint16_t, const char *, char *);
	void (*printf_throttled)(char *name, const char *format, ...);
} hmi_i;

extern hmi_i hmi;


void usart_lcd_init(void);
void HMI_send_string(char *name, char *showdata);
void HMI_printf(char *name, const char *format, ...);
void HMI_printf_throttled(char *name, const char *format, ...);
void free_lcd_data(LcdData* lcd_data);
LcdData* parse_lcd_number_data(uint8_t *data, uint16_t data_size);
LcdData *parse_lcd_string_data(uint8_t *data, uint16_t data_size);
static void HMI_send_to_slider(char *name, int num);
static void HMI_send_cmd(char *cmd);
static uint16_t HMI_get_widget_val(uint8_t *screen_data, uint16_t screen_data_size, const char *widget_name);
static void HMI_get_wight_string(uint8_t *screen_data, uint16_t screen_data_size, const char *widget_name, char *target_buffer);
static void HMI_send_curve(const char *curve_name, uint8_t channel, uint16_t val, uint16_t min_val, uint16_t max_val);
int map_val_to_360(float val, float min_val, float max_val);

#endif
