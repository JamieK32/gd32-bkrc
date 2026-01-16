#include "rc522.h"
#include "iic.h"
#include "delay.h"
#include <stdio.h>
#include "rc522_port.h"
#include "stdarg.h"
#include "rc522_user.h"

static void my_delay_ms(void *user, uint32_t ms) {
    (void)user;
    delay_ms(ms);
}

static void my_log(void *user, const char *fmt, ...) {
    (void)user;
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    printf("\r\n");
    va_end(ap);
}

IIC_Config rc522_i2c_config = {
	.scl_pin = GPIO_PIN_6,
	.scl_port = GPIOA,
	.scl_rtc = RCU_GPIOA,
	.sda_pin = GPIO_PIN_7,
	.sda_port = GPIOA,
	.sda_rtc = RCU_GPIOA,
};

rc522_t g_rc522;



void RC522_AppInit(void) {
    // 1) 初始化 IIC_Config（你原来的 InitRc522 里那段 IIC_Init）
    IIC_Init(&rc522_i2c_config);

    // 2) 装配 rc522 句柄
    g_rc522.dev_addr = 0x2F;            // 例：7-bit 地址（按你板子实际改）
    g_rc522.user     = &rc522_i2c_config;
    g_rc522.i2c_wr   = rc522_i2c_wr_cb;
    g_rc522.i2c_rd   = rc522_i2c_rd_cb;
    g_rc522.delay_ms = my_delay_ms;
    g_rc522.log      = my_log;
		
	
    
		rc522_init(&g_rc522);
}

void RC522_TestReadWrite(void) {
    rc522_key_t keyA = { .keyByte = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF} };

    char buf[17];

    rc522_status_t st = rc522_read_sector_block(&g_rc522, 1, 0, &keyA, buf);
    if (st == RC522_OK) {
        printf("READ OK: [%s]\r\n", buf);
    } else {
        printf("READ FAIL: %d\r\n", (int)st);
    }

    st = rc522_write_sector_block(&g_rc522, 1, 0, &keyA, "HELLO_RC522");
    if (st == RC522_OK) {
        printf("WRITE OK\r\n");
    } else {
        printf("WRITE FAIL: %d\r\n", (int)st);
    }
}
