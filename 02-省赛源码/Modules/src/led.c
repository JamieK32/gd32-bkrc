#include "led.h"
#include "delay.h"
#include "stdio.h"
#include "main.h"
#include "freertos.h"
#include "task.h"

led_i led = {
    .init = LED_Init,
    .test = LED_Test,
		.set = Led_Set,
		.set_all_pins = Led_Set_All_Pin,
		.state = 0,
};


static void LED_Init(void) {
  rcu_periph_clock_enable(LED_01_RTC);
  rcu_periph_clock_enable(LED_02_RTC);
  rcu_periph_clock_enable(LED_03_RTC);
  rcu_periph_clock_enable(LED_04_RTC);

  gpio_mode_set(LED_01_GPIO_Port, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLDOWN,
                LED_01_Pin);
  gpio_mode_set(LED_02_GPIO_Port, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLDOWN,
                LED_02_Pin);
  gpio_mode_set(LED_03_GPIO_Port, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLDOWN,
                LED_03_Pin);
  gpio_mode_set(LED_04_GPIO_Port, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLDOWN,
                LED_04_Pin);

  gpio_output_options_set(LED_01_GPIO_Port, GPIO_OTYPE_PP, GPIO_OSPEED_200MHZ,
                          LED_01_Pin);
  gpio_output_options_set(LED_02_GPIO_Port, GPIO_OTYPE_PP, GPIO_OSPEED_200MHZ,
                          LED_02_Pin);
  gpio_output_options_set(LED_03_GPIO_Port, GPIO_OTYPE_PP, GPIO_OSPEED_200MHZ,
                          LED_03_Pin);
  gpio_output_options_set(LED_04_GPIO_Port, GPIO_OTYPE_PP, GPIO_OSPEED_200MHZ,
                          LED_04_Pin);

  gpio_bit_write(LED_01_GPIO_Port, LED_01_Pin, RESET);
  gpio_bit_write(LED_02_GPIO_Port, LED_02_Pin, RESET);
  gpio_bit_write(LED_03_GPIO_Port, LED_03_Pin, RESET);
  gpio_bit_write(LED_04_GPIO_Port, LED_04_Pin, RESET);
}

// 全局变量
static uint16_t currentState = LED_STATE_1;
static TickType_t lastTickTime;

// LED控制函数
static void controlLED(uint16_t state) {
    gpio_bit_write(LED_01_GPIO_Port, LED_01_Pin, (state == LED_STATE_1) ? SET : RESET);
    gpio_bit_write(LED_02_GPIO_Port, LED_02_Pin, (state == LED_STATE_2) ? SET : RESET);
    gpio_bit_write(LED_03_GPIO_Port, LED_03_Pin, (state == LED_STATE_3) ? SET : RESET);
    gpio_bit_write(LED_04_GPIO_Port, LED_04_Pin, (state == LED_STATE_4) ? SET : RESET);
}

static void Led_Set(uint8_t val) {
	if (val) {
		gpio_bit_write(LED_01_GPIO_Port, LED_01_Pin, SET);
		gpio_bit_write(LED_02_GPIO_Port, LED_02_Pin, SET);
		gpio_bit_write(LED_03_GPIO_Port, LED_03_Pin, SET);
		gpio_bit_write(LED_04_GPIO_Port, LED_04_Pin, SET);
	}
	else {
		gpio_bit_write(LED_01_GPIO_Port, LED_01_Pin, RESET);
		gpio_bit_write(LED_02_GPIO_Port, LED_02_Pin, RESET);
		gpio_bit_write(LED_03_GPIO_Port, LED_03_Pin, RESET);
		gpio_bit_write(LED_04_GPIO_Port, LED_04_Pin, RESET);
	}
}

static void Led_Set_All_Pin(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
		gpio_bit_write(LED_01_GPIO_Port, LED_01_Pin, a == 0 ? RESET : SET);
		gpio_bit_write(LED_02_GPIO_Port, LED_02_Pin, b == 0 ? RESET : SET);
		gpio_bit_write(LED_03_GPIO_Port, LED_03_Pin, c == 0 ? RESET : SET);
		gpio_bit_write(LED_04_GPIO_Port, LED_04_Pin, d == 0 ? RESET : SET);
}

static void LED_Test(void) {
    // 获取当前系统时钟
    TickType_t currentTickTime = xTaskGetTickCount();

    // 检查是否达到500ms的间隔
    if (currentTickTime - lastTickTime >= pdMS_TO_TICKS(1000)) {
        // 控制当前LED
        controlLED(currentState);

        // 更新状态
        currentState = (currentState + 1) % 4;

        // 更新上次时钟记录
        lastTickTime = currentTickTime;
    }
}

