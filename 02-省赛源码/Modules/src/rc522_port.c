#include "rc522.h"
#include "iic.h"
#include "rc522_port.h"

int rc522_i2c_wr_cb(void *user, uint8_t dev_addr, uint8_t reg, uint8_t value) {
    IIC_Config *cfg = (IIC_Config*)user;

    IIC_Start(cfg);
    IIC_Send_Byte(cfg, dev_addr << 1);     // 7-bit -> 8-bit write
    if (IIC_Wait_Ack(cfg)) { IIC_Stop(cfg); return -1; }

    IIC_Send_Byte(cfg, reg);
    if (IIC_Wait_Ack(cfg)) { IIC_Stop(cfg); return -1; }

    IIC_Send_Byte(cfg, value);
    if (IIC_Wait_Ack(cfg)) { IIC_Stop(cfg); return -1; }

    IIC_Stop(cfg);
    return 0;
}

int rc522_i2c_rd_cb(void *user, uint8_t dev_addr, uint8_t reg, uint8_t *value) {
    IIC_Config *cfg = (IIC_Config*)user;

    IIC_Start(cfg);
    IIC_Send_Byte(cfg, dev_addr << 1);
    if (IIC_Wait_Ack(cfg)) { IIC_Stop(cfg); return -1; }

    IIC_Send_Byte(cfg, reg);
    if (IIC_Wait_Ack(cfg)) { IIC_Stop(cfg); return -1; }

    IIC_Start(cfg);
    IIC_Send_Byte(cfg, (dev_addr << 1) | 0x01);
    if (IIC_Wait_Ack(cfg)) { IIC_Stop(cfg); return -1; }

    *value = IIC_Read_Byte(cfg, 0);
    IIC_Stop(cfg);
    return 0;
}
