/* rgb_led_sm.c */
#include "rgb_led_sm.h"

/* ========= 内部结构 ========= */
#define LED_DEFAULT_FREQ 70.0f

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} rgb_led_rgb_t;

typedef enum {
    RGB_LED_MODE_STEADY = 0,
    RGB_LED_MODE_PWM,
    RGB_LED_MODE_BLINK,
    RGB_LED_MODE_BREATH,
    RGB_LED_MODE_TEST
} rgb_led_mode_t;

typedef struct {
    rgb_led_mode_t mode;

    /* 当前常亮颜色 */
    rgb_led_rgb_t steady_rgb;

    /* PWM 配置 */
    rgb_led_rgb_t pwm_rgb;
    float duty;      /* 0..100 */
    float freq;      /* Hz */
    bool pwm_state;  /* true=亮相位, false=灭相位 */
    TickType_t pwm_next_tick;
    TickType_t pwm_high_ticks;
    TickType_t pwm_low_ticks;

    /* Blink 配置 */
    rgb_led_rgb_t blink_rgb;
    TickType_t blink_period_ticks;
    bool blink_state;
    TickType_t blink_next_tick;

    /* Breath 配置：用 PWM duty 扫描实现 */
    rgb_led_rgb_t breath_rgb;
    uint16_t breath_duty;      /* 0..100 */
    int8_t breath_dir;         /* +1 / -1 */
    TickType_t breath_step_ticks;
    TickType_t breath_next_tick;

    /* Test 配置 */
    uint8_t test_step;         /* 0:R 1:off 2:B 3:off 4:G 5:off */
    TickType_t test_step_ticks;
    TickType_t test_next_tick;

} rgb_led_sm_t;

static rgb_led_sm_t s_led;

rgb_led_color_t rgb_led_get_color_from_rgb(uint16_t r, uint16_t g, uint16_t b)
{
    /* 阈值：>0 就当亮（适配你现在 GPIO 亮灭逻辑）
       如果你希望 0..255 下“>127 才算亮”，把 threshold 改成 127 */
    const uint16_t threshold = 0;

    uint8_t R = (r > threshold) ? 1 : 0;
    uint8_t G = (g > threshold) ? 1 : 0;
    uint8_t B = (b > threshold) ? 1 : 0;

    /* 精确映射 8 种组合 */
    if (!R && !G && !B) return RGB_LED_COLOR_OFF;
    if ( R && !G && !B) return RGB_LED_COLOR_RED;
    if (!R &&  G && !B) return RGB_LED_COLOR_GREEN;
    if (!R && !G &&  B) return RGB_LED_COLOR_BLUE;
    if (!R &&  G &&  B) return RGB_LED_COLOR_CYAN;
    if ( R &&  G && !B) return RGB_LED_COLOR_YELLOW;
    if ( R && !G &&  B) return RGB_LED_COLOR_PURPLE;
    if ( R &&  G &&  B) return RGB_LED_COLOR_WHITE;

    /* 理论上不会走到这；保底：选最强通道 */
    if (r >= g && r >= b) return RGB_LED_COLOR_RED;
    if (g >= r && g >= b) return RGB_LED_COLOR_GREEN;
    return RGB_LED_COLOR_BLUE;
}


/* ========= 内部工具 ========= */

static inline void prv_apply_rgb(rgb_led_rgb_t rgb)
{
    gpio_bit_write(LED_R_GPIO_Port, LED_R_Pin, (rgb.r == 0) ? RESET : SET);
    gpio_bit_write(LED_G_GPIO_Port, LED_G_Pin, (rgb.g == 0) ? RESET : SET);
    gpio_bit_write(LED_B_GPIO_Port, LED_B_Pin, (rgb.b == 0) ? RESET : SET);
}

static inline rgb_led_rgb_t prv_color_to_rgb(rgb_led_color_t c)
{
    rgb_led_rgb_t out = {0, 0, 0};
    switch (c) {
        case RGB_LED_COLOR_RED:    out = (rgb_led_rgb_t){1,0,0}; break;
        case RGB_LED_COLOR_GREEN:  out = (rgb_led_rgb_t){0,1,0}; break;
        case RGB_LED_COLOR_BLUE:   out = (rgb_led_rgb_t){0,0,1}; break;
        case RGB_LED_COLOR_CYAN:   out = (rgb_led_rgb_t){0,1,1}; break;
        case RGB_LED_COLOR_YELLOW: out = (rgb_led_rgb_t){1,1,0}; break;
        case RGB_LED_COLOR_PURPLE: out = (rgb_led_rgb_t){1,0,1}; break;
        case RGB_LED_COLOR_WHITE:  out = (rgb_led_rgb_t){1,1,1}; break;
        case RGB_LED_COLOR_OFF:
        default:                   out = (rgb_led_rgb_t){0,0,0}; break;
    }
    return out;
}

static inline TickType_t prv_ms_to_ticks_clamp(uint32_t ms)
{
    TickType_t t = pdMS_TO_TICKS(ms);
    return (t < 1) ? 1 : t;
}

/* 用 ms 分辨率做 PWM（tick 级别）。freq 太高会受 tick 分辨率影响 */
static void prv_recalc_pwm_ticks(void)
{
    if (s_led.freq <= 0.0f) s_led.freq = LED_DEFAULT_FREQ;
    if (s_led.duty < 0.0f) s_led.duty = 0.0f;
    if (s_led.duty > 100.0f) s_led.duty = 100.0f;

    /* period_ms = 1000/freq */
    float period_ms_f = 1000.0f / s_led.freq;
    if (period_ms_f < 1.0f) period_ms_f = 1.0f; /* 最小 1ms */

    uint32_t period_ms = (uint32_t)(period_ms_f + 0.5f);
    TickType_t period_ticks = prv_ms_to_ticks_clamp(period_ms);

    TickType_t high = (TickType_t)((period_ticks * s_led.duty) / 100.0f);
    TickType_t low  = period_ticks - high;

    if (high < 1) high = 1;
    if (low  < 1) low  = 1;

    s_led.pwm_high_ticks = high;
    s_led.pwm_low_ticks  = low;
}

/* ========= 对外 API ========= */

void rgb_led_init(void)
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

    s_led.mode = RGB_LED_MODE_STEADY;
    s_led.steady_rgb = (rgb_led_rgb_t){0,0,0};
    prv_apply_rgb(s_led.steady_rgb);
}

void rgb_led_off(void)
{
    s_led.mode = RGB_LED_MODE_STEADY;
    s_led.steady_rgb = (rgb_led_rgb_t){0,0,0};
    prv_apply_rgb(s_led.steady_rgb);
}

void rgb_led_set_rgb(uint8_t r, uint8_t g, uint8_t b)
{
    s_led.mode = RGB_LED_MODE_STEADY;
    s_led.steady_rgb = (rgb_led_rgb_t){ (r ? 1 : 0), (g ? 1 : 0), (b ? 1 : 0) };
    prv_apply_rgb(s_led.steady_rgb);
}

void rgb_led_set_color(rgb_led_color_t color)
{
    s_led.mode = RGB_LED_MODE_STEADY;
    s_led.steady_rgb = prv_color_to_rgb(color);
    prv_apply_rgb(s_led.steady_rgb);
}

void rgb_led_pwm_start(rgb_led_color_t color, float duty_percent, float freq_hz)
{
    if (duty_percent <= 0.0f) {
        rgb_led_pwm_stop();
        return;
    }

    s_led.mode = RGB_LED_MODE_PWM;
    s_led.pwm_rgb = prv_color_to_rgb(color);
    s_led.duty = duty_percent;
    s_led.freq = (freq_hz <= 0.0f) ? LED_DEFAULT_FREQ : freq_hz;

    s_led.pwm_state = false;
    prv_recalc_pwm_ticks();

    TickType_t now = xTaskGetTickCount();
    s_led.pwm_next_tick = now; /* 立即进入一次调度 */
}

void rgb_led_pwm_stop(void)
{
    if (s_led.mode == RGB_LED_MODE_PWM) {
        s_led.mode = RGB_LED_MODE_STEADY;
        s_led.steady_rgb = (rgb_led_rgb_t){0,0,0};
        prv_apply_rgb(s_led.steady_rgb);
    } else {
        /* 其他模式下也允许直接关 */
        rgb_led_off();
    }
}

void rgb_led_blink_start(rgb_led_color_t color, uint16_t period_ms)
{
    if (period_ms == 0) {
        rgb_led_off();
        return;
    }

    s_led.mode = RGB_LED_MODE_BLINK;
    s_led.blink_rgb = prv_color_to_rgb(color);
    s_led.blink_period_ticks = prv_ms_to_ticks_clamp(period_ms);

    s_led.blink_state = false;
    TickType_t now = xTaskGetTickCount();
    s_led.blink_next_tick = now;
}

void rgb_led_breath_start(rgb_led_color_t color, uint16_t step_ms)
{
    if (step_ms == 0) step_ms = 15;

    s_led.mode = RGB_LED_MODE_BREATH;
    s_led.breath_rgb = prv_color_to_rgb(color);
    s_led.breath_duty = 0;
    s_led.breath_dir = +1;

    s_led.breath_step_ticks = prv_ms_to_ticks_clamp(step_ms);

    /* Breath 用 PWM 输出 */
    s_led.duty = 0.0f;
    s_led.freq = LED_DEFAULT_FREQ;
    s_led.pwm_rgb = s_led.breath_rgb;
    s_led.pwm_state = false;
    prv_recalc_pwm_ticks();

    TickType_t now = xTaskGetTickCount();
    s_led.breath_next_tick = now;
    s_led.pwm_next_tick = now;
}

void rgb_led_test_start(uint16_t step_ms)
{
    if (step_ms == 0) step_ms = 500;

    s_led.mode = RGB_LED_MODE_TEST;
    s_led.test_step = 0;
    s_led.test_step_ticks = prv_ms_to_ticks_clamp(step_ms);

    TickType_t now = xTaskGetTickCount();
    s_led.test_next_tick = now;
}

/* ========= 状态机 tick：所有效果都在这里跑 ========= */

void rgb_led_ticks(void)
{
    TickType_t now = xTaskGetTickCount();

    switch (s_led.mode) {

    case RGB_LED_MODE_STEADY:
        /* 常亮模式无需周期处理 */
        break;

    case RGB_LED_MODE_PWM:
        if ((TickType_t)(now - s_led.pwm_next_tick) < (TickType_t)0x80000000u) {
            /* 到点了 */
            if (s_led.duty <= 0.0f) {
                rgb_led_pwm_stop();
                break;
            }

            /* PWM 相位切换 */
            if (s_led.pwm_state) {
                prv_apply_rgb((rgb_led_rgb_t){0,0,0});
                s_led.pwm_next_tick = now + s_led.pwm_low_ticks;
            } else {
                prv_apply_rgb(s_led.pwm_rgb);
                s_led.pwm_next_tick = now + s_led.pwm_high_ticks;
            }
            s_led.pwm_state = !s_led.pwm_state;
        }
        break;

    case RGB_LED_MODE_BLINK:
        if ((TickType_t)(now - s_led.blink_next_tick) < (TickType_t)0x80000000u) {
            s_led.blink_state = !s_led.blink_state;
            s_led.blink_next_tick = now + s_led.blink_period_ticks;
            prv_apply_rgb(s_led.blink_state ? s_led.blink_rgb : (rgb_led_rgb_t){0,0,0});
        }
        break;

    case RGB_LED_MODE_BREATH:
        /* breath duty 扫描：每 step_ticks 更新一次 duty */
        if ((TickType_t)(now - s_led.breath_next_tick) < (TickType_t)0x80000000u) {
            s_led.breath_next_tick = now + s_led.breath_step_ticks;

            /* 更新 duty */
            if (s_led.breath_dir > 0) {
                if (s_led.breath_duty >= 100) {
                    s_led.breath_dir = -1;
                } else {
                    s_led.breath_duty++;
                }
            } else {
                if (s_led.breath_duty == 0) {
                    s_led.breath_dir = +1;
                } else {
                    s_led.breath_duty--;
                }
            }

            s_led.duty = (float)s_led.breath_duty;
            s_led.pwm_rgb = s_led.breath_rgb;
            prv_recalc_pwm_ticks();

            /* 立即让 PWM 逻辑接管下一次切换 */
            s_led.pwm_next_tick = now;
        }

        /* 用 PWM 逻辑输出（不阻塞） */
        if ((TickType_t)(now - s_led.pwm_next_tick) < (TickType_t)0x80000000u) {
            if (s_led.duty <= 0.0f) {
                prv_apply_rgb((rgb_led_rgb_t){0,0,0});
                s_led.pwm_next_tick = now + s_led.pwm_low_ticks;
                s_led.pwm_state = false;
                break;
            }

            if (s_led.pwm_state) {
                prv_apply_rgb((rgb_led_rgb_t){0,0,0});
                s_led.pwm_next_tick = now + s_led.pwm_low_ticks;
            } else {
                prv_apply_rgb(s_led.pwm_rgb);
                s_led.pwm_next_tick = now + s_led.pwm_high_ticks;
            }
            s_led.pwm_state = !s_led.pwm_state;
        }
        break;

    case RGB_LED_MODE_TEST:
        if ((TickType_t)(now - s_led.test_next_tick) < (TickType_t)0x80000000u) {
            s_led.test_next_tick = now + s_led.test_step_ticks;

            /* 序列：R on -> off -> B on -> off -> G on -> off -> 循环 */
            switch (s_led.test_step) {
                case 0: prv_apply_rgb((rgb_led_rgb_t){1,0,0}); break;
                case 1: prv_apply_rgb((rgb_led_rgb_t){0,0,0}); break;
                case 2: prv_apply_rgb((rgb_led_rgb_t){0,0,1}); break;
                case 3: prv_apply_rgb((rgb_led_rgb_t){0,0,0}); break;
                case 4: prv_apply_rgb((rgb_led_rgb_t){0,1,0}); break;
                case 5: prv_apply_rgb((rgb_led_rgb_t){0,0,0}); break;
                default: prv_apply_rgb((rgb_led_rgb_t){0,0,0}); break;
            }

            s_led.test_step++;
            if (s_led.test_step > 5) s_led.test_step = 0;
        }
        break;

    default:
        /* 容错：回到 off */
        rgb_led_off();
        break;
    }
}
