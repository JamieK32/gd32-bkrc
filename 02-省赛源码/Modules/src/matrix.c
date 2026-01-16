#include "matrix.h"

// ======= 扫描数据（你原来就是这样） =======
static uint16_t col_data[8] = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};

// ======= 内部状态机 =======
typedef enum {
    MATRIX_STATE_CLEAR = 0,
    MATRIX_STATE_TIMER,
    MATRIX_STATE_SCROLL,
    MATRIX_STATE_IMG,
} matrix_state_t;

static volatile matrix_state_t g_state = MATRIX_STATE_CLEAR;

// ======= 显示帧缓冲：8列，每列1字节（与你原 display_static 的 data[8] 对齐） =======
static uint8_t g_frame[8] = {0};

// ======= 扫描指针：每次 Tick 刷新一列 =======
static uint8_t g_scan_col = 0;

// ======= Tick 节奏参数（简单易用：默认 1秒计数、滚动每50ms走一步） =======
#ifndef MATRIX_TIMER_STEP_MS
#define MATRIX_TIMER_STEP_MS   1000u
#endif

#ifndef MATRIX_SCROLL_STEP_MS
#define MATRIX_SCROLL_STEP_MS  50u
#endif

// ======= 计时(计数)参数 =======
static struct {
    uint8_t dir;        // MATRIX_COUNT_UP / DOWN
    uint8_t start;
    uint8_t end;
    uint8_t current;
    uint32_t last_ms;
} g_timer = { .dir = MATRIX_COUNT_UP, .start = 0, .end = 9, .current = 0, .last_ms = 0};

// ======= 滚动参数 =======
static struct {
    uint8_t *data;
    uint16_t len;
    uint8_t direction;
    int32_t  pos;        // 新增：消息左边缘在显示窗口中的位置（可为负）
    uint32_t last_ms;
    uint8_t  loop;
    uint16_t speed_ms;
		uint8_t vpos;
} g_scroll = { .loop = 1, .speed_ms = MATRIX_SCROLL_STEP_MS,  .vpos = 0};


// ======= 图片参数 =======
static struct {
    uint8_t *data; // 默认8字节
} g_img = {0};

#ifndef MATRIX_VSCROLL_MAX
#define MATRIX_VSCROLL_MAX 320u   // 够用：len最大建议 <= 304（因为要 +16 空白）
#endif

static uint8_t  g_vrows[MATRIX_VSCROLL_MAX]; // 竖向“行流”，每个元素是 1 行（8bit宽）
static uint16_t g_vrows_len = 0;             // 行流总行数
static int32_t  g_vpos = 0;                  // 当前窗口顶行索引

// 把“横向列序列 data[len]（len=字数*8）”转换为“竖向行流 rows[]（每行8bit）”
// 竖排顺序：毛(上) -> 主 -> 席 -> 万 -> 岁(下)
// rows 前后各加 8 行 0，用于进/出屏
static void Build_VerticalRows_FromCols(const uint8_t *data, uint16_t len)
{
    // len 必须是 8 的倍数（每字8列）
    len = (uint16_t)(len - (len % 8));
    uint16_t chars = len / 8;

    // 总行数 = 顶部空白8 + 每字8行*chars + 底部空白8
    uint16_t total = (uint16_t)(8 + chars * 8 + 8);
    if (total > MATRIX_VSCROLL_MAX) {
        // 超出缓存就截断字数
        chars = (uint16_t)((MATRIX_VSCROLL_MAX - 16) / 8);
        total = (uint16_t)(8 + chars * 8 + 8);
        len = (uint16_t)(chars * 8);
    }

    // 顶部空白
    for (uint16_t i = 0; i < 8; i++) g_vrows[i] = 0x00;

    // 逐字：把 8列（列序列）转成 8行（行序列），再叠到竖向行流里
    // 注意：这里假设 glyph_col[x] 的 bit0 对应第0行（可能是最上或最下，取决于你字库/接线）
    // 如果你发现上下颠倒：把 (1u<<y) 改成 (1u<<(7-y))
    for (uint16_t ch = 0; ch < chars; ch++) {
        const uint8_t *glyph_col = &data[ch * 8];

        for (uint8_t y = 0; y < 8; y++) {
            uint8_t row_bits = 0;
            for (uint8_t x = 0; x < 8; x++) {
                if (glyph_col[x] & (1u << y)) {
                    row_bits |= (1u << x);
                    // 如果你发现左右镜像：改成 row_bits |= (1u << (7 - x));
                }
            }
            g_vrows[8 + ch * 8 + y] = row_bits;
        }
    }

    // 底部空白
    for (uint16_t i = 0; i < 8; i++) g_vrows[8 + chars * 8 + i] = 0x00;

    g_vrows_len = total;
}


// ======= 你必须提供一个“当前毫秒”的来源 =======
// 最简：用 delay.c 里的 systick 计数/或者你自己封装的 millis()
// 这里给一个弱定义，你可以在别的文件里实现同名函数覆盖。
__attribute__((weak)) uint32_t Matrix_Millis(void) {
    // 如果你没有毫秒计数器，就先用 delay_ms 的systick变量替代（需要你按工程实际改）
    // 推荐：实现一个真正的 millis()，否则计时/滚动节奏会不准。
    return (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
}

// ======= 74HC595: 合并数据（高位列 低位行，且行数据取反） =======
static uint16_t HC595_Dat_Handle(uint8_t dat, uint8_t cnt)
{
    uint16_t rt = cnt;
    rt <<= 8;
    rt |= (uint8_t)(~dat);
    return rt;
}

// ======= 低层：刷新某一列（从 g_frame 取列数据） =======
static void Matrix_Refresh_OneColumn(uint8_t col)
{
    uint16_t dat = HC595_Dat_Handle(g_frame[col], (uint8_t)col_data[col]);
    HC595_Send_16Bit(dat);
}

// ======= 字库装载：0~9 单数字（使用你原来的 matrix_nums_data: 10*8） =======
static void Matrix_LoadDigit_0_9(uint8_t digit)
{
    if (digit > 9) digit = 0;
    memcpy(g_frame, matrix_data_9 + digit * 8, 8);
}

// ======= 字库装载：0~99 两位（使用你原来的 matrix_data_2：每个数字4列，共8列拼接） =======
// 兼容你原来 CountUp_99 的做法：temp_buffer[8] = high(4) + low(4)
static void Matrix_LoadNumber_0_99(uint8_t num)
{
    uint8_t low  = num % 10;
    uint8_t high = (num / 10) % 10;

    // matrix_data_2[x] 必须是 4 字节宽（与你原工程一致）
    memcpy(&g_frame[0], matrix_data_99[high], 4);
    memcpy(&g_frame[4], matrix_data_99[low], 4);
}

// ======= Timer：每秒推进一次 =======
static void Matrix_Timer_Update(uint32_t now_ms)
{
    // 先把当前值装载到帧缓冲（不阻塞）
    if (g_timer.start <= 9 && g_timer.end <= 9) {
        Matrix_LoadDigit_0_9(g_timer.current);
    } else {
        Matrix_LoadNumber_0_99(g_timer.current);
    }

    // 到点才更新 current
    if ((uint32_t)(now_ms - g_timer.last_ms) < MATRIX_TIMER_STEP_MS) return;
    g_timer.last_ms = now_ms;

    if (g_timer.dir == MATRIX_COUNT_UP) {
        if (g_timer.current < g_timer.end) g_timer.current++;
        else g_timer.current = g_timer.start; // 简单：循环
    } else { // DOWN
          if (g_timer.current > g_timer.start) g_timer.current--;
					else g_timer.current = g_timer.end;   // 到 start 后回到 end 循环
    }
}


static void Matrix_Scroll_Update(uint32_t now_ms)
{
    if (!g_scroll.data || g_scroll.len == 0) {
        memset(g_frame, 0, 8);
        return;
    }

    // =========================
    // 竖向滚动：T2B / B2T
    // =========================
    if (g_scroll.direction == MATRIX_SCROLL_T2B || g_scroll.direction == MATRIX_SCROLL_B2T) {

        // 1) 取当前8行窗口（rows），转成 8列（g_frame）
        // rows[r] 的 bitx 表示第x列是否亮；我们要把8行拼成每列一个字节
        for (uint8_t x = 0; x < 8; x++) {
            uint8_t col_bits = 0;
            for (uint8_t y = 0; y < 8; y++) {
                int32_t ry = g_vpos + y;
                uint8_t row = (ry >= 0 && ry < (int32_t)g_vrows_len) ? g_vrows[ry] : 0x00;

                if (row & (1u << x)) {
                    col_bits |= (1u << y);
                    // 如果你发现上下颠倒：改成 col_bits |= (1u << (7 - y));
                }
            }
            g_frame[x] = col_bits;
        }

        // 2) 节奏推进
        if ((uint32_t)(now_ms - g_scroll.last_ms) < g_scroll.speed_ms) return;
        g_scroll.last_ms = now_ms;

        if (g_scroll.direction == MATRIX_SCROLL_T2B) {
            g_vpos++;
            if (g_vpos > (int32_t)(g_vrows_len - 8)) {
                if (g_scroll.loop) g_vpos = 0;
            }
        } else { // B2T
            g_vpos--;
            if (g_vpos < 0) {
                if (g_scroll.loop) g_vpos = (int32_t)(g_vrows_len - 8);
            }
        }

        return;
    }

    // =========================
    // 横向滚动：L2R / R2L（保留你原来的pos映射）
    // =========================
    for (uint8_t x = 0; x < 8; x++) {
        int32_t m = (int32_t)x - g_scroll.pos;
        g_frame[x] = (m >= 0 && m < (int32_t)g_scroll.len) ? g_scroll.data[m] : 0x00;
    }

    if ((uint32_t)(now_ms - g_scroll.last_ms) < g_scroll.speed_ms) return;
    g_scroll.last_ms = now_ms;

    if (g_scroll.direction == MATRIX_SCROLL_R2L) {
        g_scroll.pos++;
        if (g_scroll.pos > 8) {
            if (g_scroll.loop) g_scroll.pos = -(int32_t)g_scroll.len;
        }
    } else if (g_scroll.direction == MATRIX_SCROLL_L2R) {
				g_scroll.pos--;
        if (g_scroll.pos < -(int32_t)g_scroll.len) {
            if (g_scroll.loop) g_scroll.pos = 8;
        }
    }
}



// ======= Img：直接装载8字节图片到 g_frame =======
static void Matrix_Img_Update(void)
{
    if (!g_img.data) {
        memset(g_frame, 0, 8);
        return;
    }
    memcpy(g_frame, g_img.data, 8);
}

// ============================================================================
// 74HC595 底层（你原来的，几乎不动）
// ============================================================================
void HC595_Init(void)
{
    rcu_periph_clock_enable(RCLK_RTC);
    rcu_periph_clock_enable(SCLK_RTC);
    rcu_periph_clock_enable(SER_RTC);
    rcu_periph_clock_enable(OE_RTC);

    gpio_mode_set(RCLK_GPIO_Port, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLDOWN, RCLK_Pin);
    gpio_mode_set(SCLK_GPIO_Port, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLDOWN, SCLK_Pin);
    gpio_mode_set(SER_GPIO_Port,  GPIO_MODE_OUTPUT, GPIO_PUPD_PULLDOWN, SER_Pin);
    gpio_mode_set(OE_GPIO_Port,   GPIO_MODE_OUTPUT, GPIO_PUPD_PULLDOWN, OE_Pin);

    gpio_output_options_set(RCLK_GPIO_Port, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, RCLK_Pin);
    gpio_output_options_set(SCLK_GPIO_Port, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, SCLK_Pin);
    gpio_output_options_set(SER_GPIO_Port,  GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, SER_Pin);
    gpio_output_options_set(OE_GPIO_Port,   GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, OE_Pin);

    gpio_bit_write(OE_GPIO_Port, OE_Pin, RESET);

    RCLK_L;
    SCLK_L;
    SER_L;
    delay_ms(10);
}

void HC595_Send_16Bit(uint16_t data)
{
    for (uint16_t i = 0; i < 16; i++)
    {
        if (data & 0x8000) SER_H;
        else              SER_L;

        delay_us(1);
        SCLK_L;
        delay_us(1);
        SCLK_H;
        delay_us(1);

        data <<= 1;
    }

    RCLK_L;
    delay_us(1);
    RCLK_H;
}

// ============================================================================
// Matrix API（状态入口 + Tick）
// ============================================================================
void Matrix_Init(void)
{
    HC595_Init();
    Matrix_Clear();
}

void Matrix_Clear(void)
{
    memset(g_frame, 0, 8);
    g_state = MATRIX_STATE_CLEAR;
}

void Matrix_TimerState(uint8_t direction, uint8_t restart, uint8_t start, uint8_t end)
{
    g_timer.dir   = (direction == MATRIX_COUNT_DOWN) ? MATRIX_COUNT_DOWN : MATRIX_COUNT_UP;
    g_timer.start = start;
    g_timer.end   = end;

    // 简单保护：如果 start/end 反了，自动交换
    if (g_timer.start > g_timer.end) {
        uint8_t t = g_timer.start;
        g_timer.start = g_timer.end;
        g_timer.end = t;
    }

    if (restart) {
			g_timer.current = (g_timer.dir == MATRIX_COUNT_DOWN) ? g_timer.end : g_timer.start;
			g_timer.last_ms = Matrix_Millis();
    }

    g_state = MATRIX_STATE_TIMER;
}

void Matrix_ScrollStateEx(uint8_t *data, uint16_t len, uint8_t direction,
                          uint8_t loop, uint16_t speed_ms)
{
    g_scroll.data = data;
    g_scroll.len  = len;
    g_scroll.direction = direction;
    g_scroll.loop = loop ? 1 : 0;
    g_scroll.speed_ms = (speed_ms == 0) ? 1 : speed_ms;
    g_scroll.last_ms = Matrix_Millis();
    g_scroll.vpos = 0;

    if (direction == MATRIX_SCROLL_R2L) {
				g_scroll.pos = -(int32_t)len;
        g_state = MATRIX_STATE_SCROLL;
        return;
    }
    if (direction == MATRIX_SCROLL_L2R) {
        g_scroll.pos = 8;
        g_state = MATRIX_STATE_SCROLL;
        return;
    }

    // ======= 竖向滚动：构建行流 =======
    if (direction == MATRIX_SCROLL_T2B || direction == MATRIX_SCROLL_B2T) {
        Build_VerticalRows_FromCols(data, len);

        if (g_vrows_len < 8) {
            g_vrows_len = 8;
        }

        if (direction == MATRIX_SCROLL_T2B) {
            g_vpos = 0;                       // 从顶端空白开始，向下推进
        } else {
            g_vpos = (int32_t)(g_vrows_len - 8); // 从底端空白开始，向上推进
        }

        g_state = MATRIX_STATE_SCROLL;
        return;
    }

    g_scroll.pos = 0;
    g_state = MATRIX_STATE_SCROLL;
}


void Matrix_ScrollState(uint8_t *data, uint16_t len, uint8_t direction)
{
	Matrix_ScrollStateEx(data, len, direction, 1, MATRIX_SCROLL_STEP_MS);
}

void Matrix_ImgState(uint8_t *data)
{
    g_img.data = data;
    g_state = MATRIX_STATE_IMG;
}

// Tick：
// 1) 根据状态更新 g_frame（按节奏推进）
// 2) 刷新 1 列（列扫描）
// 建议：每 1ms~2ms 调一次（越快越亮）
void Matrix_Tick(void)
{
    uint32_t now_ms = Matrix_Millis();

    switch (g_state) {
        case MATRIX_STATE_TIMER:
            Matrix_Timer_Update(now_ms);
            break;

        case MATRIX_STATE_SCROLL:
            Matrix_Scroll_Update(now_ms);
            break;

        case MATRIX_STATE_IMG:
            Matrix_Img_Update();
            break;

        case MATRIX_STATE_CLEAR:
        default:
            // g_frame 已是0
            break;
    }

    // 列扫描刷新
    Matrix_Refresh_OneColumn(g_scan_col);
    g_scan_col++;
    if (g_scan_col >= 8) g_scan_col = 0;
}
