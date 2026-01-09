#ifndef __mpu6050_H
#define __mpu6050_H

#include "gd32f4xx.h"
#include "math.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "delay.h"
#include "iic.h"
#include "oled.h"

#define MPU_SELF_TESTX_REG		0X0D	// 自检X轴寄存器
#define MPU_SELF_TESTY_REG		0X0E	// 自检Y轴寄存器
#define MPU_SELF_TESTZ_REG		0X0F	// 自检Z轴寄存器
#define MPU_SELF_TESTA_REG		0X10	// 自检A轴寄存器
#define MPU_SAMPLE_RATE_REG		0X19	// 采样率寄存器
#define MPU_CFG_REG				0X1A	// 配置寄存器
#define MPU_GYRO_CFG_REG		0X1B	// 陀螺仪配置寄存器
#define MPU_ACCEL_CFG_REG		0X1C	// 加速度计配置寄存器
#define MPU_MOTION_DET_REG		0X1F	// 运动检测配置寄存器
#define MPU_FIFO_EN_REG			0X23	// FIFO使能寄存器
#define MPU_I2CMST_CTRL_REG		0X24	// I2C主控寄存器
#define MPU_I2CSLV0_ADDR_REG	0X25	// I2C从机0地址寄存器
#define MPU_I2CSLV0_REG			0X26	// I2C从机0数据寄存器
#define MPU_I2CSLV0_CTRL_REG	0X27	// I2C从机0控制寄存器
#define MPU_I2CSLV1_ADDR_REG	0X28	// I2C从机1地址寄存器
#define MPU_I2CSLV1_REG			0X29	// I2C从机1数据寄存器
#define MPU_I2CSLV1_CTRL_REG	0X2A	// I2C从机1控制寄存器
#define MPU_I2CSLV2_ADDR_REG	0X2B	// I2C从机2地址寄存器
#define MPU_I2CSLV2_REG			0X2C	// I2C从机2数据寄存器
#define MPU_I2CSLV2_CTRL_REG	0X2D	// I2C从机2控制寄存器
#define MPU_I2CSLV3_ADDR_REG	0X2E	// I2C从机3地址寄存器
#define MPU_I2CSLV3_REG			0X2F	// I2C从机3数据寄存器
#define MPU_I2CSLV3_CTRL_REG	0X30	// I2C从机3控制寄存器
#define MPU_I2CSLV4_ADDR_REG	0X31	// I2C从机4地址寄存器
#define MPU_I2CSLV4_REG			0X32	// I2C从机4数据寄存器
#define MPU_I2CSLV4_DO_REG		0X33	// I2C从机4输出数据寄存器
#define MPU_I2CSLV4_CTRL_REG	0X34	// I2C从机4控制寄存器
#define MPU_I2CSLV4_DI_REG		0X35	// I2C从机4输入数据寄存器
#define MPU_I2CMST_STA_REG		0X36	// I2C主控状态寄存器
#define MPU_INTBP_CFG_REG		0X37	// 中断配置寄存器
#define MPU_INT_EN_REG			0X38	// 中断使能寄存器
#define MPU_INT_STA_REG			0X3A	// 中断状态寄存器
#define MPU_ACCEL_XOUTH_REG		0X3B	// 加速度计X轴高8位寄存器
#define MPU_ACCEL_XOUTL_REG		0X3C	// 加速度计X轴低8位寄存器
#define MPU_ACCEL_YOUTH_REG		0X3D	// 加速度计Y轴高8位寄存器
#define MPU_ACCEL_YOUTL_REG		0X3E	// 加速度计Y轴低8位寄存器
#define MPU_ACCEL_ZOUTH_REG		0X3F	// 加速度计Z轴高8位寄存器
#define MPU_ACCEL_ZOUTL_REG		0X40	// 加速度计Z轴低8位寄存器
#define MPU_TEMP_OUTH_REG		0X41	// 温度传感器高8位寄存器
#define MPU_TEMP_OUTL_REG		0X42	// 温度传感器低8位寄存器
#define MPU_GYRO_XOUTH_REG		0X43	// 陀螺仪X轴高8位寄存器
#define MPU_GYRO_XOUTL_REG		0X44	// 陀螺仪X轴低8位寄存器
#define MPU_GYRO_YOUTH_REG		0X45	// 陀螺仪Y轴高8位寄存器
#define MPU_GYRO_YOUTL_REG		0X46	// 陀螺仪Y轴低8位寄存器
#define MPU_GYRO_ZOUTH_REG		0X47	// 陀螺仪Z轴高8位寄存器
#define MPU_GYRO_ZOUTL_REG		0X48	// 陀螺仪Z轴低8位寄存器
#define MPU_I2CSLV0_DO_REG		0X63	// I2C从机0数据输出寄存器
#define MPU_I2CSLV1_DO_REG		0X64	// I2C从机1数据输出寄存器
#define MPU_I2CSLV2_DO_REG		0X65	// I2C从机2数据输出寄存器
#define MPU_I2CSLV3_DO_REG		0X66	// I2C从机3数据输出寄存器
#define MPU_I2CMST_DELAY_REG	0X67	// I2C延时寄存器
#define MPU_SIGPATH_RST_REG		0X68	// 信号路径复位寄存器
#define MPU_MDETECT_CTRL_REG	0X69	// 运动检测控制寄存器
#define MPU_USER_CTRL_REG		0X6A	// 用户控制寄存器
#define MPU_PWR_MGMT1_REG		0X6B	// 电源管理1寄存器
#define MPU_PWR_MGMT2_REG		0X6C	// 电源管理2寄存器 
#define MPU_FIFO_CNTH_REG		0X72	// FIFO计数高8位寄存器
#define MPU_FIFO_CNTL_REG		0X73	// FIFO计数低8位寄存器
#define MPU_FIFO_RW_REG			0X74	// FIFO读写寄存器
#define MPU_DEVICE_ID_REG		0X75	// 设备ID寄存器
 
// 如果AD0接地(I2C地址为0x68)，否则为0x69
#define MPU_ADDR				0X68

typedef struct {
    short angle_x;
    short angle_y;
    short angle_z;
} mpu_data;

typedef struct mpu6050_t {
	void (*init)(void);
	void (*get_data)(mpu_data *);
	short (*get_temperature)(void);
	uint8_t (*get_gyroscope)(short *, short *, short *);
	void (*test)(void);
} mpu6050_t; 

extern mpu6050_t mpu6050;

static uint8_t MPU_Init(void); 								// 初始化MPU6050
static uint8_t MPU_Read_Len(uint8_t addr,uint8_t reg,uint8_t len,uint8_t *buf);  // IIC读取多个字节
static uint8_t MPU_Write_Byte(uint8_t reg,uint8_t data);				// IIC写入一个字节
static uint8_t MPU_Read_Byte(uint8_t reg);						// IIC读取一个字节
static uint8_t MPU_Set_Gyro_Fsr(uint8_t fsr);   					 // 设置陀螺仪全量程范围
static uint8_t MPU_Set_Accel_Fsr(uint8_t fsr);   					 // 设置加速度计全量程范围
static uint8_t MPU_Set_LPF(uint16_t lpf);        					 // 设置低通滤波器
static uint8_t MPU_Set_Rate(uint16_t rate);      					 // 设置数据输出速率
static uint8_t MPU_Set_Fifo(uint8_t sens);       					 // 设置FIFO
static int MPU6050_Get_Angle(float x,float y,float z,uint8_t dir);		 // 获取角度
static short MPU_Get_Temperature(void);          					 // 获取温度
static uint8_t MPU_Get_Gyroscope(short *gx,short *gy,short *gz); 		 // 获取陀螺仪数据
static uint8_t MPU_Get_Accelerometer(short *ax,short *ay,short *az); 	 // 获取加速度计数据
static void MPU6050_Hardware_Init(void);							 // 硬件初始化
static void MPU6050_Software_Init(void);							 // 软件初始化
static void MPU6050_Init(void);									 // 完整初始化
static void MPU6050_Test(void);									 // 测试MPU6050
static void MPU_GetData(mpu_data* data);							 // 获取MPU6050数据信息

#endif


