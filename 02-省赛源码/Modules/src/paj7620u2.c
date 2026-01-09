/*
 * paj7620u2.c
 *
 * created: 10/1/2025
 *  author:
 */

#include "paj7620u2.h"
#include "paj7620u2_cfg.h"
#include "main.h"
#include "delay.h"

paj7620_t paj7620 = {
    .init = PAJ7620_Init,
    .get_gesture = Get_Gesture,
};

IIC_Config PAJ7620_I2c_Cfg = {
	.scl_pin = GPIO_PIN_1,
	.scl_port = GPIOC,
	.scl_rtc = RCU_GPIOC,
	.sda_pin = GPIO_PIN_0,
	.sda_port = GPIOC,
	.sda_rtc = RCU_GPIOC,
};

// PAJ7620U2写一个字节数据
unsigned char I2C_Write_Byte(IIC_Config *cfg, unsigned char REG_Address, unsigned char REG_data)
{
    IIC_Start(cfg);
    IIC_Send_Byte(cfg, PAJ7620_ID);
    if (IIC_Wait_Ack(cfg))
    {
        IIC_Stop(cfg); // 释放总线
        return 1;              // 没应答则退出
    }
    IIC_Send_Byte(cfg, REG_Address);
    IIC_Wait_Ack(cfg);
    IIC_Send_Byte(cfg, REG_data);
    IIC_Wait_Ack(cfg);
    IIC_Stop(cfg);

    return 0;
}

// PAJ7620U2读一个字节数据
unsigned char I2C_Read_Byte(IIC_Config *cfg, unsigned char REG_Address)
{
    unsigned char REG_data;

    IIC_Start(cfg);
    IIC_Send_Byte(cfg, PAJ7620_ID); // 发写命令
    if (IIC_Wait_Ack(cfg))
    {
        IIC_Stop(cfg); // 释放总线
        return 0;              // 没应答则退出
    }
    IIC_Send_Byte(cfg, REG_Address);
    IIC_Wait_Ack(cfg);
    IIC_Start(cfg);
    IIC_Send_Byte(cfg, PAJ7620_ID | 0x01); // 发读命令
    IIC_Wait_Ack(cfg);
    REG_data = IIC_Read_Byte(cfg, 0);
    IIC_Stop(cfg);

    return REG_data;
}

// PAJ7620U2读n个字节数据
unsigned char I2C_Read_nByte(IIC_Config *cfg, unsigned char REG_Address, unsigned short len, unsigned char *buf)
{
    IIC_Start(cfg);
    IIC_Send_Byte(cfg, PAJ7620_ID); // 发写命令
    if (IIC_Wait_Ack(cfg))
    {
        IIC_Stop(cfg); // 释放总线
        return 1;              // 没应答则退出
    }
    IIC_Send_Byte(cfg, REG_Address);
    IIC_Wait_Ack(cfg);

    IIC_Start(cfg);
    IIC_Send_Byte(cfg, PAJ7620_ID | 0x01); // 发读命令
    IIC_Wait_Ack(cfg);
    while (len)
    {
        if (len == 1)
        {
            *buf = IIC_Read_Byte(cfg, 0);
        }
        else
        {
            *buf = IIC_Read_Byte(cfg, 1);
        }
        buf++;
        len--;
    }
    IIC_Stop(cfg); // 释放总线

    return 0;
}

// PAJ7620唤醒
void I2C_WakeUp(IIC_Config *cfg)
{
    IIC_Start(cfg);
    IIC_Send_Byte(cfg, PAJ7620_ID); // 发写命令
    IIC_Stop(cfg);                  // 释放总线
}

void paj7620u2_selectBank(bank_e bank)
{
    switch (bank)
    {
    case BANK0:
        I2C_Write_Byte(&PAJ7620_I2c_Cfg, PAJ_REGITER_BANK_SEL, PAJ_BANK0);
        break; // BANK0寄存器区域
    case BANK1:
        I2C_Write_Byte(&PAJ7620_I2c_Cfg, PAJ_REGITER_BANK_SEL, PAJ_BANK1);
        break; // BANK1寄存器区域
    }
}

// PAJ7620U2唤醒
unsigned char paj7620u2_wakeup(void)
{
    unsigned char data = 0x0a;
    I2C_WakeUp(&PAJ7620_I2c_Cfg);                 // 唤醒PAJ7620U2
    delay_ms(5);                                 // 唤醒时间>400us
    I2C_WakeUp(&PAJ7620_I2c_Cfg);                 // 唤醒PAJ7620U2
    delay_ms(5);                                 // 唤醒时间>400us
    paj7620u2_selectBank(BANK0);                 // 进入BANK0寄存器区域
    data = I2C_Read_Byte(&PAJ7620_I2c_Cfg, 0x00); // 读取状态
    if (data != 0x20)
        return 0; // 唤醒失败

    return 1;
}

// PAJ7620U2初始化
// 返回值：0:失败 1:成功
unsigned char paj7620u2_init(void)
{
    unsigned char status;
    IIC_Init(&PAJ7620_I2c_Cfg); // IIC初始化
    status = paj7620u2_wakeup();        // 唤醒PAJ7620U2
    if (!status)
        return 0;
    paj7620u2_selectBank(BANK0); // 进入BANK0寄存器区域
    for (unsigned char i = 0; i < INIT_SIZE; i++)
    {
        I2C_Write_Byte(&PAJ7620_I2c_Cfg, init_Array[i][0], init_Array[i][1]); // 初始化PAJ7620U2
    }
    paj7620u2_selectBank(BANK0); // 切换回BANK0寄存器区域
    return 1;
}

void PAJ7620_Init(void)
{
    while (!paj7620u2_init()) // PAJ7620U2传感器初始化
    {
        LOG_W("initializing paj7620"); // 使用你的 LOG_W 宏，如果没用请替换或删除
    }
    paj7620u2_selectBank(BANK0); // 进入BANK0寄存器区域
    for (uint8_t i = 0; i < GESTURE_SIZE; i++)
    {
        I2C_Write_Byte(&PAJ7620_I2c_Cfg, gesture_arry[i][0], gesture_arry[i][1]); // 手势识别模式初始化
    }
    paj7620u2_selectBank(BANK0); // 切换回BANK0寄存器区域
}


/* 手势正确执行对应的任务 */
unsigned short Get_Gesture(void)
{
    unsigned short gesture_data = 0;
    unsigned char status = 1;
    unsigned char data[2] = {0x00};
		if (paj7620.log == NULL) {
			LOG_I("paj7620 log callback is not set"); // Fallback log if callback is not set, using LOG_I (you can modify this)
		}
    status = I2C_Read_nByte(&PAJ7620_I2c_Cfg, PAJ_GET_INT_FLAG1, 2, &data[0]); // 读取手势状态
    if (!status)
    {
        gesture_data = (unsigned char)data[1] << 8 | data[0];
				if (paj7620.gesture_call_back != NULL) paj7620.gesture_call_back(gesture_data);
        switch (gesture_data)
        {
        case GES_UP: // 向上
            paj7620.log("GES_UP"); // 使用你的 LOG_I 宏，如果没用请替换或删除
            // 执行向上手势的任务
            break;
        case GES_DOWN: // 向下
            paj7620.log("GES_DOWN");
            // 执行向下手势的任务
            break;
        case GES_LEFT: // 向左
            paj7620.log("GES_LEFT");
            // 执行向左手势的任务
            break;
        case GES_RIGHT: // 向右
            paj7620.log("GES_RIGHT");
            // 执行向右手势的任务
            break;
        case GES_FORWARD: // 向前
            paj7620.log("GES_FORWARD");
            // 执行向前手势的任务
            break;
        case GES_BACKWARD: // 向后
            paj7620.log("GES_BACKWARD");
            // 执行向后手势的任务
            break;
        case GES_CLOCKWISE: // 顺时针
            paj7620.log("GES_CLOCKWISE");
            // 执行顺时针手势的任务
            break;
        case GES_COUNT_CLOCKWISE: // 逆时针
            paj7620.log("GES_COUNT_CLOCKWISE");
            // 执行逆时针手势的任务
            break;
        case GES_WAVE: // 挥动
            paj7620.log("GES_WAVE");
            // 执行挥动手势的任务
            break;
        default:
            paj7620.log("unknown gestrue");
            break;
        }
        gesture_data = 0;
    }
    return gesture_data;
}
