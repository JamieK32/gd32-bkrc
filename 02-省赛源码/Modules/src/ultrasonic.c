#include "ultrasonic.h"
#include "delay.h"
#include "oled.h"
#include <stdint.h>

#define Ultrasonic_OUT_RTC RCU_GPIOA
#define Ultrasonic_OUT_GPIO_Port GPIOA
#define Ultrasonic_OUT_Pin GPIO_PIN_12

#define Ultrasonic_IN_RTC RCU_GPIOA
#define Ultrasonic_IN_GPIO_Port GPIOA
#define Ultrasonic_IN_Pin GPIO_PIN_11

/* 你原来的宏定义（别动） */
#define EXTI_x_IRQn EXTI10_15_IRQn
#define EXTI_SOURCE_GPIOx EXTI_SOURCE_GPIOA
#define EXTI_SOURCE_PINx EXTI_SOURCE_PIN12
#define EXTI_x EXTI_12
#define EXTI_IRQHandler EXTI10_15_IRQHandler

/* ====== 参数（按你稳定版本习惯） ====== */
#define TRIG_US 10u
#define RETRIGGER_US 60000u // 建议 60ms 防串音
#define TIMEOUT_US 30000u   // 30ms 超时

/* 10us tick：TIMER1 每次更新中断 = 10us（arr=10-1, psc=...） */
#define TICK_US 10u
#define US_TO_TICK(us) ((uint32_t)((us) / TICK_US))

/* ====== 状态机 ====== */
typedef enum
{
    U_IDLE = 0,
    U_WAIT_ECHO,
    U_DONE,
    U_TIMEOUT
} u_state_t;

static volatile u_state_t g_state = U_IDLE;

/* 10us 系统时间（只增不减） */
static volatile uint32_t g_tick10us = 0;

/* 本次测距的起始 tick、结束 tick */
static volatile uint32_t g_t_start = 0;
static volatile uint32_t g_t_end = 0;

static volatile uint8_t g_sample_ready = 0;

/* 上一次触发时间，用于 retrigger */
static uint32_t g_last_trig_tick = 0;

/* 你原版的稳定算法变量 */
static float g_output_cm = 0.0f;

/* 你原来的全局也可以保留，但不再依赖 status 计数了 */
volatile uint32_t real_time = 0;
volatile float dis_temp = 0;

static float g_hist_mm[3] = {0};
static uint8_t g_hist_index = 0;

/* ====== 你已有的外部函数（初始化不动） ====== */
void my_EXTI_Init(void)
{
    rcu_periph_clock_enable(Ultrasonic_OUT_RTC);
    rcu_periph_clock_enable(RCU_SYSCFG);

    gpio_mode_set(Ultrasonic_OUT_GPIO_Port, GPIO_MODE_INPUT, GPIO_PUPD_NONE, Ultrasonic_OUT_Pin);

    nvic_irq_enable(EXTI_x_IRQn, 2U, 0U);

    syscfg_exti_line_config(EXTI_SOURCE_GPIOx, EXTI_SOURCE_PINx);

    exti_init(EXTI_x, EXTI_INTERRUPT, EXTI_TRIG_FALLING);
    exti_interrupt_flag_clear(EXTI_x);
}

void TIMER1_init(uint16_t arr, uint16_t psc)
{
    timer_parameter_struct timer_initpara;

    rcu_periph_clock_enable(RCU_TIMER1);
    rcu_timer_clock_prescaler_config(RCU_TIMER_PSC_MUL4);
    timer_struct_para_init(&timer_initpara);
    timer_deinit(TIMER1);

    timer_initpara.period = arr;
    timer_initpara.prescaler = psc;
    timer_initpara.alignedmode = TIMER_COUNTER_CENTER_DOWN;
    timer_initpara.counterdirection = TIMER_COUNTER_UP;
    timer_initpara.clockdivision = TIMER_CKDIV_DIV1;
    timer_initpara.repetitioncounter = 0;
    timer_init(TIMER1, &timer_initpara);

    timer_auto_reload_shadow_enable(TIMER1);
    timer_interrupt_enable(TIMER1, TIMER_INT_UP);
    nvic_irq_enable(TIMER1_IRQn, 0, 1);
}

/* ====== TRIG 脉冲（按你原来） ====== */
static inline void Ultrasonic_Send_Pulse(void)
{
    gpio_bit_set(Ultrasonic_IN_GPIO_Port, Ultrasonic_IN_Pin);
    delay_us(TRIG_US);
    gpio_bit_reset(Ultrasonic_IN_GPIO_Port, Ultrasonic_IN_Pin);
}

/* ====== 硬件初始化（按你原来） ====== */
void Ultrasonic_Hardware_Init(void)
{
    rcu_periph_clock_enable(Ultrasonic_IN_RTC);
    gpio_mode_set(Ultrasonic_IN_GPIO_Port, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLDOWN, Ultrasonic_IN_Pin);
    gpio_output_options_set(Ultrasonic_IN_GPIO_Port, GPIO_OTYPE_PP, GPIO_OSPEED_200MHZ, Ultrasonic_IN_Pin);
}

/* ====== 软件初始化（不改 timer/exti init 内容，只改调用逻辑） ====== */
void Ultrasonic_Software_Init(void)
{
    TIMER1_init(10 - 1, 168 - 1); // 你原来的配置（不动）
    my_EXTI_Init();               // 你原来的配置（不动）

    /* 关键：让 TIMER1 一直跑，提供 g_tick10us 的时间基准 */
    timer_enable(TIMER1);
}

/* ====== 对外初始化 ====== */
void Ultrasonic_Init(void)
{
    Ultrasonic_Hardware_Init();
    Ultrasonic_Software_Init();

    g_state = U_IDLE;
    g_tick10us = 0;
    g_sample_ready = 0;
    g_last_trig_tick = 0;

    g_output_cm = 0.0f;
}

/* ====== 状态机轮询：放 main while(1) 里一直调用 ====== */
void Ultrasonic_Task(void)
{
    uint32_t now = g_tick10us;

    switch (g_state)
    {
    case U_IDLE:
        /* 到间隔才触发 */
        if ((now - g_last_trig_tick) >= US_TO_TICK(RETRIGGER_US))
        {
            g_last_trig_tick = now;

            g_sample_ready = 0;
            g_t_start = now;

            Ultrasonic_Send_Pulse();
            g_state = U_WAIT_ECHO;
        }
        break;

    case U_WAIT_ECHO:
        /* 等 EXTI 把 sample_ready 置 1 */
        if (g_sample_ready)
        {
            g_sample_ready = 0;

            uint32_t dt_tick = (g_t_end - g_t_start); // 10us tick
            real_time = dt_tick;                      // 兼容你原来的变量

						dis_temp = (float)dt_tick * 1.7f - 2.0f; // mm
						if (dis_temp < 0)
								dis_temp = 0;

						/* === 中值滤波部分 === */
						g_hist_mm[g_hist_index] = dis_temp;
						g_hist_index = (g_hist_index + 1) % 3;

						float a = g_hist_mm[0];
						float b = g_hist_mm[1];
						float c = g_hist_mm[2];
						float median;

						/* 手写快速中值求法（3个值） */
						if ((a >= b && a <= c) || (a <= b && a >= c))
								median = a;
						else if ((b >= a && b <= c) || (b <= a && b >= c))
								median = b;
						else
								median = c;

						/* 单位转换 mm → cm */
						g_output_cm = (median / 10.0f) / 1.209f; // 保留你原校准

            g_state = U_DONE;
        }
        else
        {
            /* 超时 */
            if ((now - g_t_start) > US_TO_TICK(TIMEOUT_US))
            {
                g_state = U_TIMEOUT;
            }
        }
        break;

    case U_DONE:
        /* 下一轮交给 retrigger 控制 */
        g_state = U_IDLE;
        break;

    case U_TIMEOUT:
    default:
        /* 超时就丢弃本次，回到 IDLE 等下一轮 */
        g_state = U_IDLE;
        break;
    }
}

/* ====== 获取距离（cm）：不阻塞，返回最新平均结果 ====== */
float Ultrasonic_Get_Cm(void)
{
    /* 你也可以只在 main 里调用 Ultrasonic_Task，这里顺手调用一次也行 */
    Ultrasonic_Task();
    return g_output_cm;
}

/* ====== OLED 测试 ====== */
void Ultrasonic_Test(void)
{
    Ultrasonic_Task();
    oled.print(10 * 1, 4, 16, "%5.1f CM    ", g_output_cm);
}

/* ====== TIMER1 中断：每 10us 来一次，累加系统 tick ====== */
void TIMER1_IRQHandler(void)
{
    if (SET == timer_interrupt_flag_get(TIMER1, TIMER_INT_FLAG_UP))
    {
        timer_interrupt_flag_clear(TIMER1, TIMER_INT_FLAG_UP);
        g_tick10us++;
    }
}

/* ====== EXTI FALLING：锁存结束时间（不改 init，只改 handler） ====== */
void EXTI_IRQHandler(void)
{
    if (RESET != exti_interrupt_flag_get(EXTI_x))
    {

        /* 只在等待回波时才接收 */
        if (g_state == U_WAIT_ECHO)
        {
            g_t_end = g_tick10us;
            g_sample_ready = 1;
        }
    }
    exti_interrupt_flag_clear(EXTI_x);
}
