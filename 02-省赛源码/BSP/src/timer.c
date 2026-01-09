#include "timer.h"

void pwm_gpio_config(void) {
  rcu_periph_clock_enable(FAN_PWM_RTC);

  gpio_mode_set(FAN_PWM_GPIO_Port, GPIO_MODE_AF, GPIO_PUPD_NONE, FAN_PWM_Pin);
  gpio_output_options_set(FAN_PWM_GPIO_Port, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ,
                          FAN_PWM_Pin);

  gpio_af_set(FAN_PWM_GPIO_Port, GPIO_AF_2, FAN_PWM_Pin);
}

void TIMER1_init(uint16_t arr, uint16_t psc)
{
    timer_parameter_struct timer_initpara;

    rcu_periph_clock_enable(RCU_TIMER1);
    rcu_timer_clock_prescaler_config(RCU_TIMER_PSC_MUL4);
    timer_struct_para_init(&timer_initpara);
    timer_deinit(TIMER1);

    timer_initpara.period = arr;  // 10-1，10us 溢出一次
    timer_initpara.prescaler = psc;  // 168-1，1MHz 计数频率
    timer_initpara.alignedmode = TIMER_COUNTER_CENTER_DOWN;
    timer_initpara.counterdirection = TIMER_COUNTER_UP;
    timer_initpara.clockdivision = TIMER_CKDIV_DIV1;
    timer_initpara.repetitioncounter = 0;
    timer_init(TIMER1, &timer_initpara);

    timer_auto_reload_shadow_enable(TIMER1);
    timer_interrupt_enable(TIMER1, TIMER_INT_UP);  // 使能溢出中断
    nvic_irq_enable(TIMER1_IRQn, 0, 1);  // 配置中断优先级
}


//系统微秒延时 TIMER 弃用
void TIMER3_init(uint16_t arr, uint16_t psc) {
  timer_parameter_struct timer_initpara;

  nvic_priority_group_set(NVIC_PRIGROUP_PRE2_SUB2);
  rcu_periph_clock_enable(RCU_TIMER3);
  rcu_timer_clock_prescaler_config(RCU_TIMER_PSC_MUL4);
  timer_struct_para_init(&timer_initpara);
  timer_deinit(TIMER3);

  timer_initpara.period = arr;
  timer_initpara.prescaler = psc;
  timer_initpara.alignedmode = TIMER_COUNTER_EDGE;
  timer_initpara.counterdirection = TIMER_COUNTER_UP;
  timer_initpara.clockdivision = TIMER_CKDIV_DIV1;
  timer_initpara.repetitioncounter = 0;
  timer_init(TIMER3, &timer_initpara);

  timer_auto_reload_shadow_enable(TIMER3);
  timer_interrupt_enable(TIMER3, TIMER_INT_UP);
  nvic_irq_enable(TIMER3_IRQn, 2, 1);

  timer_disable(TIMER3);
}

//风扇 TIMER
void TIMER4_init(uint16_t arr, uint16_t psc) {
  timer_oc_parameter_struct timer_ocintpara;
  timer_parameter_struct timer_initpara;

  rcu_periph_clock_enable(RCU_TIMER4);
  rcu_timer_clock_prescaler_config(RCU_TIMER_PSC_MUL4);

  timer_deinit(TIMER4);

  /* TIMER4 configuration */
  timer_initpara.prescaler = psc;
  timer_initpara.period = arr;
  timer_initpara.alignedmode = TIMER_COUNTER_EDGE;
  timer_initpara.counterdirection = TIMER_COUNTER_UP;
  timer_initpara.clockdivision = TIMER_CKDIV_DIV1;
  timer_initpara.repetitioncounter = 0;
  timer_init(TIMER4, &timer_initpara);

  /* CH0 configuration in PWM mode */
  timer_ocintpara.ocpolarity = TIMER_OC_POLARITY_HIGH;
  timer_ocintpara.outputstate = TIMER_CCX_ENABLE;
  timer_ocintpara.ocnpolarity = TIMER_OCN_POLARITY_HIGH;
  timer_ocintpara.outputnstate = TIMER_CCXN_DISABLE;
  timer_ocintpara.ocidlestate = TIMER_OC_IDLE_STATE_LOW;
  timer_ocintpara.ocnidlestate = TIMER_OCN_IDLE_STATE_LOW;

  timer_channel_output_config(TIMER4, TIMER_CH_0, &timer_ocintpara);

  /* CH0 configuration in PWM mode1,duty cycle 0% */
  timer_channel_output_pulse_value_config(TIMER4, TIMER_CH_0, 500);
  timer_channel_output_mode_config(TIMER4, TIMER_CH_0, TIMER_OC_MODE_PWM0);
  timer_channel_output_shadow_config(TIMER4, TIMER_CH_0,
                                     TIMER_OC_SHADOW_DISABLE);

  /* auto-reload preload enable */
  timer_auto_reload_shadow_enable(TIMER4);
  /* auto-reload preload enable */
  timer_enable(TIMER4);
}
