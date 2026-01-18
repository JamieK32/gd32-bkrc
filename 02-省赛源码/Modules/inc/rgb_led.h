#ifndef __RGB_LED_H__
#define __RGB_LED_H__

#include "gd32f4xx.h"
#include "freertos.h"
#include "task.h"
#include "timers.h"

#define LED_B_RTC 		RCU_GPIOB
#define LED_B_GPIO_Port	GPIOB
#define LED_B_Pin 		GPIO_PIN_3

#define LED_G_RTC 		RCU_GPIOB
#define LED_G_GPIO_Port	GPIOB
#define LED_G_Pin 		GPIO_PIN_4

#define LED_R_RTC 		RCU_GPIOB
#define LED_R_GPIO_Port	GPIOB
#define LED_R_Pin 		GPIO_PIN_5

#define LED_DEFAULT_FREQ 70

// 定义颜色结构体
typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} Color;

#define COLOR_RED    (Color){1, 0, 0}
#define COLOR_GREEN  (Color){0, 1, 0}
#define COLOR_BLUE   (Color){0, 0, 1}
#define COLOR_CYAN   (Color){0, 1, 1}
#define COLOR_YELLOW (Color){1, 1, 0}
#define COLOR_PURPLE (Color){1, 0, 1}
#define COLOR_WHITE  (Color){1, 1, 1}
#define COLOR_OFF    (Color){0, 0, 0}

typedef struct {
    Color color;
    float duty;
    float frq;
} LedPwmConfig_t;

//控制RGB_LED接口
typedef struct rgb_led_i {
    void (*init)(void);
    void (*test)(void);
    void (*set_rgb)(uint8_t, uint8_t, uint8_t);
		void (*blink_control)(Color, uint16_t);
    void (*set_color)(Color);
    void (*off)(void);
    void (*set_pwm_timer)(Color, float, float);
		void (*stop_pwm_timer)(void);
} rgb_led_i;


static void RGB_LED_Init(void);
static void LED_Test(void);
static void LED_Set_Rgb(uint8_t R, uint8_t G, uint8_t B);
static void LED_Set_Color(Color color);
static void LED_OFF(void);
static void LED_Set_Color_TimerPwm(Color color, float duty, float frq);
static void LED_Stop_TimerPwm(void);
static void LED_TimerPwm_Callback(TimerHandle_t xTimer);
static void LED_Blink_Control(Color color, uint16_t period_ms);
static void LED_Breath(Color c);

extern rgb_led_i rgb_led;


#endif
