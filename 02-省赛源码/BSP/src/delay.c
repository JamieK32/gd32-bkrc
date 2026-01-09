#include "delay.h"
#include "gd32f4xx.h"
#include "FreeRTOS.h"
#include "task.h"

static uint8_t g_dwt_inited = 0;

static inline void dwt_init_once(void)
{
    if (!g_dwt_inited) {
        CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
        DWT->CYCCNT = 0;
        DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
        g_dwt_inited = 1;
    } else {
        if ((DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk) == 0) {
            DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
        }
    }
}

static inline void delay_us_busy(uint32_t us)
{
    if (us == 0) return;
    dwt_init_once();

    uint32_t start = DWT->CYCCNT;
    uint32_t ticks = (uint32_t)((uint64_t)us * (SystemCoreClock / 1000000UL));

    while ((uint32_t)(DWT->CYCCNT - start) < ticks) {
        __NOP();
    }
}

void delay_us(uint32_t us)
{
    if (us >= 1000) {
        uint32_t ms = us / 1000;
        us %= 1000;

        if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING &&
            xPortIsInsideInterrupt() == pdFALSE) {
            vTaskDelay(pdMS_TO_TICKS(ms));
        } else {
            delay_us_busy(ms * 1000UL);
        }
    }

    if (us > 0) {
        delay_us_busy(us);
    }
}

void delay_ms(uint32_t ms)
{
    if (ms == 0) return;

    if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING &&
        xPortIsInsideInterrupt() == pdFALSE) {
        vTaskDelay(pdMS_TO_TICKS(ms));
    } else {
        delay_us_busy(ms * 1000UL);
    }
}
