#include "step_motor.h"

// 连接P24端口

static uint8_t speed = 90;

void Step_Hardware_Init(void) {
  rcu_periph_clock_enable(STEP_A_RTC);
  rcu_periph_clock_enable(STEP_B_RTC);
  rcu_periph_clock_enable(STEP_C_RTC);
  rcu_periph_clock_enable(STEP_D_RTC);

  gpio_mode_set(STEP_A_GPIO_Port, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLDOWN,
                STEP_A_Pin);
  gpio_mode_set(STEP_B_GPIO_Port, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLDOWN,
                STEP_B_Pin);
  gpio_mode_set(STEP_C_GPIO_Port, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLDOWN,
                STEP_C_Pin);
  gpio_mode_set(STEP_D_GPIO_Port, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLDOWN,
                STEP_D_Pin);

  gpio_output_options_set(STEP_A_GPIO_Port, GPIO_OTYPE_PP, GPIO_OSPEED_200MHZ,
                          STEP_A_Pin);
  gpio_output_options_set(STEP_B_GPIO_Port, GPIO_OTYPE_PP, GPIO_OSPEED_200MHZ,
                          STEP_B_Pin);
  gpio_output_options_set(STEP_C_GPIO_Port, GPIO_OTYPE_PP, GPIO_OSPEED_200MHZ,
                          STEP_C_Pin);
  gpio_output_options_set(STEP_D_GPIO_Port, GPIO_OTYPE_PP, GPIO_OSPEED_200MHZ,
                          STEP_D_Pin);

  gpio_bit_reset(STEP_A_GPIO_Port, STEP_A_Pin);
  gpio_bit_reset(STEP_B_GPIO_Port, STEP_B_Pin);
  gpio_bit_reset(STEP_C_GPIO_Port, STEP_C_Pin);
  gpio_bit_reset(STEP_D_GPIO_Port, STEP_D_Pin);
}

void set_step(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
  STEPA(a);
  STEPB(b);
  STEPC(c);
  STEPD(d);
}

void step_delay(void) { 
	if (100 - speed) delay_ms(100 - speed); 
	else delay_ms(1);
}

void step_motor_set_speed(uint8_t s) {
	if (s >= 100) speed = 100;
	if (s <= 0) speed = 0;
} 

void Reverse(uint16_t time) {
  uint8_t steps[8][4] = {
      {1, 0, 0, 1}, // DA
      {0, 0, 0, 1}, // D
      {0, 1, 0, 1}, // BD
      {0, 1, 0, 0}, // B
      {0, 1, 1, 0}, // CB
      {0, 0, 1, 0}, // C
      {1, 0, 1, 0}, // AC
      {1, 0, 0, 0}  // A
  };

  while (time--) {
    for (int i = 0; i < 8; i++) {
      set_step(steps[i][0], steps[i][1], steps[i][2], steps[i][3]);
      step_delay();
    }
  }
}

void Corotation(uint16_t time) {
  uint8_t steps[8][4] = {
      {1, 0, 0, 0}, // A
      {1, 0, 1, 0}, // AC
      {0, 0, 1, 0}, // C
      {0, 1, 1, 0}, // CB
      {0, 1, 0, 0}, // B
      {0, 1, 0, 1}, // BD
      {0, 0, 0, 1}, // D
      {1, 0, 0, 1}  // DA
  };
  while (time--) {
    for (int i = 0; i < 8; i++) {
      set_step(steps[i][0], steps[i][1], steps[i][2], steps[i][3]);
      step_delay();
    }
  }
}

void STOP(void) { set_step(0, 0, 0, 0); }

step_motor_i step_motor = {
    .init = Step_Hardware_Init,
    .corotation = Corotation,
    .reverse = Reverse,
    .stop = STOP,
		.set_speed = step_motor_set_speed,
};
