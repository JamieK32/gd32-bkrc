#include "bh1750.h"
#include "iic.h"
#include <stdbool.h>

#include "freertos.h"
#include "task.h"

// 连接 P20 端口

static uint8_t  BUF[2];
static uint16_t Lx_value = 0;

IIC_Config bh1750_i2c_init_struct;

/* =========================
 * 可调参数（按需改）
 * ========================= */

// BH1750 命令间隔（原来每次 Cmd_Write 后 delay_ms(10)）
#define BH_CMD_GAP_MS          10

// BH1750 测量等待时间（你原来 300~400ms）
// 一般 One-Time H-Resolution Mode 需要约 120~180ms（不同设置不同）
// 你如果稳定用 300ms 也可以
#define BH_MEASURE_WAIT_MS     180

// 自动采样周期：比如每 500ms 采一次（你按需求改）
#define BH_SAMPLE_PERIOD_MS    300


/* =========================
 * 状态机定义
 * ========================= */

typedef enum {
    BH_ST_IDLE = 0,
    BH_ST_CMD_ON,
    BH_ST_CMD_RESET,
    BH_ST_CMD_ONE,
    BH_ST_WAIT_MEASURE,
    BH_ST_READ,
    BH_ST_CONVERT,
    BH_ST_DONE,
    BH_ST_ERROR
} BH1750_State_t;

static BH1750_State_t bh_state = BH_ST_IDLE;

static TickType_t bh_deadline = 0;      // 状态推进的时间门限
static TickType_t bh_next_sample = 0;   // 下次自动采样的时间点

static bool bh_ready = false;           // 是否有新数据（一次性标志）

/* =========================
 * 小工具
 * ========================= */

static inline TickType_t ms_to_ticks(uint32_t ms)
{
    return pdMS_TO_TICKS(ms);
}

static inline bool time_reached(TickType_t now, TickType_t deadline)
{
    // tick 溢出安全判断：now - deadline 在无符号下仍正确
    return (TickType_t)(now - deadline) < (TickType_t)0x80000000u;
}

/* =========================
 * I2C 访问（短同步，不做 delay）
 * 注意：这里假设 IIC_Wait_Ack 返回 0 表示 ACK 收到
 *       如果你库相反（1 表示 ACK），把判断取反即可
 * ========================= */

static inline bool Cmd_Write_BH1750_NoDelay(uint8_t cmd)
{
    IIC_Start(&bh1750_i2c_init_struct);

    IIC_Send_Byte(&bh1750_i2c_init_struct, BH1750_Addr + 0);
    if (IIC_Wait_Ack(&bh1750_i2c_init_struct)) { IIC_Stop(&bh1750_i2c_init_struct); return false; }

    IIC_Send_Byte(&bh1750_i2c_init_struct, cmd);
    if (IIC_Wait_Ack(&bh1750_i2c_init_struct)) { IIC_Stop(&bh1750_i2c_init_struct); return false; }

    IIC_Stop(&bh1750_i2c_init_struct);
    return true;
}

static inline bool Read_BH1750_NoDelay(void)
{
    IIC_Start(&bh1750_i2c_init_struct);

    IIC_Send_Byte(&bh1750_i2c_init_struct, BH1750_Addr + 1);
    if (IIC_Wait_Ack(&bh1750_i2c_init_struct)) { IIC_Stop(&bh1750_i2c_init_struct); return false; }

    BUF[0] = IIC_Read_Byte(&bh1750_i2c_init_struct, 1);
    BUF[1] = IIC_Read_Byte(&bh1750_i2c_init_struct, 0);

    IIC_Stop(&bh1750_i2c_init_struct);
    return true;
}

static inline void Convert_BH1750(void)
{
    uint16_t raw = ((uint16_t)BUF[0] << 8) | BUF[1];

    // 原逻辑：lux = raw / 1.2
    // 用整数近似：raw * 10 / 12 ≈ raw / 1.2
    Lx_value = (uint16_t)((raw * 10u) / 12u);
}

/* =========================
 * 对外 API
 * ========================= */

static void BH1750_Init(void)
{
    bh1750_i2c_init_struct.scl_pin  = GPIO_PIN_5;
    bh1750_i2c_init_struct.scl_port = GPIOE;
    bh1750_i2c_init_struct.scl_rtc  = RCU_GPIOE;

    bh1750_i2c_init_struct.sda_pin  = GPIO_PIN_6;
    bh1750_i2c_init_struct.sda_port = GPIOE;
    bh1750_i2c_init_struct.sda_rtc  = RCU_GPIOE;

    IIC_Init(&bh1750_i2c_init_struct);

    bh_state = BH_ST_IDLE;
    bh_ready = false;

    TickType_t now = xTaskGetTickCount();
    bh_deadline = now;
    bh_next_sample = now; // init 后立刻来一次采样
}

static void BH1750_Tick(void)
{
    TickType_t now = xTaskGetTickCount();

    // IDLE 状态：到点了就启动一次采样序列
    if (bh_state == BH_ST_IDLE)
    {
        if (time_reached(now, bh_next_sample))
        {
            // 启动采样序列
            bh_state = BH_ST_CMD_ON;
            bh_deadline = now; // 立即执行
        }
        else
        {
            return; // 没到采样时间，直接退出
        }
    }

    // 状态推进：未到 deadline 不推进
    if (!time_reached(now, bh_deadline)) return;

    switch (bh_state)
    {
        case BH_ST_CMD_ON:
            if (!Cmd_Write_BH1750_NoDelay(BH1750_ON)) {
                bh_state = BH_ST_ERROR;
                break;
            }
            bh_state = BH_ST_CMD_RESET;
            bh_deadline = now + ms_to_ticks(BH_CMD_GAP_MS);
            break;

        case BH_ST_CMD_RESET:
            if (!Cmd_Write_BH1750_NoDelay(BH1750_RSET)) {
                bh_state = BH_ST_ERROR;
                break;
            }
            bh_state = BH_ST_CMD_ONE;
            bh_deadline = now + ms_to_ticks(BH_CMD_GAP_MS);
            break;

        case BH_ST_CMD_ONE:
            if (!Cmd_Write_BH1750_NoDelay(BH1750_ONE)) {
                bh_state = BH_ST_ERROR;
                break;
            }
            bh_state = BH_ST_WAIT_MEASURE;
            bh_deadline = now + ms_to_ticks(BH_MEASURE_WAIT_MS);
            break;

        case BH_ST_WAIT_MEASURE:
            bh_state = BH_ST_READ;
            bh_deadline = now; // 立即读
            break;

        case BH_ST_READ:
            if (!Read_BH1750_NoDelay()) {
                bh_state = BH_ST_ERROR;
                break;
            }
            bh_state = BH_ST_CONVERT;
            bh_deadline = now; // 立即转换
            break;

        case BH_ST_CONVERT:
            Convert_BH1750();
            bh_state = BH_ST_DONE;
            bh_deadline = now;
            break;

        case BH_ST_DONE:
            // 一次采样完成：置 ready，并安排下次采样
            bh_ready = true;

            // 下次采样时间 = 当前时间 + 周期
            bh_next_sample = now + ms_to_ticks(BH_SAMPLE_PERIOD_MS);

            // 回到空闲
            bh_state = BH_ST_IDLE;
            break;

        case BH_ST_ERROR:
        default:
            // 出错：不置 ready，安排稍后重试（避免死循环狂打 I2C）
            bh_ready = false;
            bh_next_sample = now + ms_to_ticks(BH_SAMPLE_PERIOD_MS);
            bh_state = BH_ST_IDLE;
            break;
    }
}

static bool BH1750_IsReady(void)
{
    return bh_ready;
}

static uint16_t BH1750_Get_Lx(void)
{
    // 取数据后，把 ready 清掉（典型 one-shot ready 语义）
    bh_ready = false;
    return Lx_value;
}

/* =========================
 * 导出接口结构体
 * ========================= */

bh1750_i bh1750 = {
    .init      = BH1750_Init,
    .tick      = BH1750_Tick,
    .is_ready  = BH1750_IsReady,
    .get       = BH1750_Get_Lx,
    .port_name = "P20",
};
