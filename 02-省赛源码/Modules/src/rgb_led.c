#include "rgb_led.h"
#include "freertos.h"
#include "task.h"
#include "timers.h"
#include "stdbool.h"
#include "delay.h"

//连接P24端口

rgb_led_i rgb_led =  {
    .init = RGB_LED_Init,
    .test = LED_Test,
    .set_rgb = LED_Set_Rgb,
    .set_color = LED_Set_Color,
    .off = LED_OFF,
    .set_pwm_timer = LED_Set_Color_TimerPwm,
		.stop_pwm_timer = LED_Stop_TimerPwm,
		.breath = LED_Breath,
		.blink_control = LED_Blink_Control,
};

static void RGB_LED_Init(void)
{
	rcu_periph_clock_enable(LED_B_RTC);
	rcu_periph_clock_enable(LED_G_RTC);
	rcu_periph_clock_enable(LED_R_RTC);
	

	gpio_mode_set(LED_B_GPIO_Port, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLDOWN, LED_B_Pin);
	gpio_mode_set(LED_G_GPIO_Port, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLDOWN, LED_G_Pin);
	gpio_mode_set(LED_R_GPIO_Port, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLDOWN, LED_R_Pin);
	
	
	gpio_output_options_set(LED_B_GPIO_Port, GPIO_OTYPE_PP, GPIO_OSPEED_200MHZ, LED_B_Pin);	
	gpio_output_options_set(LED_G_GPIO_Port, GPIO_OTYPE_PP, GPIO_OSPEED_200MHZ, LED_G_Pin);
	gpio_output_options_set(LED_R_GPIO_Port, GPIO_OTYPE_PP, GPIO_OSPEED_200MHZ, LED_R_Pin);
	
	gpio_bit_write(LED_B_GPIO_Port, LED_B_Pin, RESET);
	gpio_bit_write(LED_G_GPIO_Port, LED_G_Pin, RESET);
	gpio_bit_write(LED_R_GPIO_Port, LED_R_Pin, RESET);
}


static void LED_OFF(void) {
	gpio_bit_write(LED_B_GPIO_Port, LED_B_Pin, RESET);
	gpio_bit_write(LED_G_GPIO_Port, LED_G_Pin, RESET);
	gpio_bit_write(LED_R_GPIO_Port, LED_R_Pin, RESET);
}

static void LED_Test(void)
 {
	gpio_bit_write(LED_R_GPIO_Port, LED_R_Pin, SET); // LED_R_Pin ON
	vTaskDelay(pdMS_TO_TICKS(500));
	gpio_bit_write(LED_R_GPIO_Port, LED_R_Pin, RESET); // LED_R_Pin OFF
	vTaskDelay(pdMS_TO_TICKS(500));
	gpio_bit_write(LED_B_GPIO_Port, LED_B_Pin, SET); // LED_B_Pin ON
	vTaskDelay(pdMS_TO_TICKS(500));
	gpio_bit_write(LED_B_GPIO_Port, LED_B_Pin, RESET); // LED_B_Pin OFF
	vTaskDelay(pdMS_TO_TICKS(500));
	gpio_bit_write(LED_G_GPIO_Port, LED_G_Pin, SET); // LED_G_Pin ON
	vTaskDelay(pdMS_TO_TICKS(500));
	gpio_bit_write(LED_G_GPIO_Port, LED_G_Pin, RESET); // LED_G_Pin OFF
	vTaskDelay(pdMS_TO_TICKS(500));
}

static void LED_Set_Rgb(uint8_t R, uint8_t G, uint8_t B) {
		gpio_bit_write(LED_R_GPIO_Port, LED_R_Pin, R == 0 ? RESET : SET);
    gpio_bit_write(LED_G_GPIO_Port, LED_G_Pin, G == 0 ? RESET : SET);
    gpio_bit_write(LED_B_GPIO_Port, LED_B_Pin, B == 0 ? RESET : SET);
}

static void LED_Set_Color(Color color) {
    LED_Set_Rgb(color.r, color.g, color.b);
}

static TimerHandle_t ledTimerHandle = NULL;
static LedPwmConfig_t ledConfig;
static bool ledState = false;  // LED 当前状态（亮/灭）

static void LED_Timer_CreateOrRestart(TickType_t initialPeriod) {
    if (ledTimerHandle != NULL) {
        xTimerStop(ledTimerHandle, 0);
        xTimerDelete(ledTimerHandle, 0);
    }

    ledTimerHandle = xTimerCreate("LED_TimerPwm", initialPeriod, pdFALSE, NULL, LED_TimerPwm_Callback);

    if (ledTimerHandle != NULL) {
        xTimerStart(ledTimerHandle, 0);
    }
}

static void LED_Set_Color_TimerPwm(Color color, float duty, float frq) {
    // 更新 LED 配置
    ledConfig.color = color;
    ledConfig.duty = duty;
    ledConfig.frq = frq;

    ledState = false; // 初始状态

    if (duty == 0.0f) {
        LED_Stop_TimerPwm();
        return;
    }

    LED_Timer_CreateOrRestart(pdMS_TO_TICKS(1)); // 创建或重新启动定时器
}

static void LED_TimerPwm_Callback(TimerHandle_t xTimer) {
    if (ledConfig.duty <= 0.0f) {
        LED_Stop_TimerPwm();
        return;
    }

    TickType_t period = pdMS_TO_TICKS(1000 / ledConfig.frq);
    TickType_t highTime = (TickType_t)((period * ledConfig.duty) / 100.0f);
    TickType_t lowTime = period - highTime;

    if (highTime < pdMS_TO_TICKS(1)) highTime = pdMS_TO_TICKS(1);
    if (lowTime < pdMS_TO_TICKS(1)) lowTime = pdMS_TO_TICKS(1);

    if (ledState) {
        LED_Set_Color(COLOR_OFF);
        xTimerChangePeriod(ledTimerHandle, lowTime, 0);
    } else {
        LED_Set_Color(ledConfig.color);
        xTimerChangePeriod(ledTimerHandle, highTime, 0);
    }

    ledState = !ledState;
}

static void LED_Stop_TimerPwm(void) {
    if (ledTimerHandle != NULL) {
        xTimerStop(ledTimerHandle, 0);
        LED_Set_Color(COLOR_OFF);
    }
}

static void LED_Breath(Color c) {
	for (int i = 0; i <= 100; i++) {
		rgb_led.set_pwm_timer(c, i, LED_DEFAULT_FREQ);
		delay_ms(15);
	}
	for (int i = 100; i >= 0; i--) {
		rgb_led.set_pwm_timer(c, i, LED_DEFAULT_FREQ);
		delay_ms(15);
	}
}

#include "beep.h"

static void LED_Blink_Control(Color color, uint16_t period_ms)
{
    static TickType_t led_blink_period = 0;  // 上次切换时间
    static uint8_t led_state = 0;            // LED 状态

    // 如果周期为 0，则关闭 LED 并停止闪烁
    if (period_ms == 0) {
        rgb_led.off();
        return;
    }

    // 获取当前 tick 计数
    TickType_t current_ticks = xTaskGetTickCount();

    // 检查是否到达切换时间
    if (current_ticks - led_blink_period >= pdMS_TO_TICKS(period_ms)) {
        led_state = !led_state;              // 切换状态
        led_blink_period = current_ticks;    // 更新上次切换时间
    }

    // 根据状态设置 LED
    if (led_state) {
        if (color.r) rgb_led.set_color(color);
				beep.control(1);
    } else {
        rgb_led.set_color(COLOR_OFF);
				beep.control(0);
    }
}
