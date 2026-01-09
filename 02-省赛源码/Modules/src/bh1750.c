#include "bh1750.h"
#include "iic.h"
#include "oled.h"
#include "delay.h"

//连接P20端口

static uint8_t BUF[2];
static uint16_t Lx_value = 0;

IIC_Config bh1750_i2c_init_struct;

static void BH1750_Init(void)
{
	bh1750_i2c_init_struct.scl_pin = GPIO_PIN_5;
	bh1750_i2c_init_struct.scl_port = GPIOE;
	bh1750_i2c_init_struct.scl_rtc = RCU_GPIOE;
	bh1750_i2c_init_struct.sda_pin = GPIO_PIN_6;
	bh1750_i2c_init_struct.sda_port = GPIOE;
	bh1750_i2c_init_struct.sda_rtc = RCU_GPIOE;
	IIC_Init(&bh1750_i2c_init_struct);
}

static inline void Cmd_Write_BH1750(uint8_t cmd)
{
	IIC_Start(&bh1750_i2c_init_struct);
	IIC_Send_Byte(&bh1750_i2c_init_struct, BH1750_Addr + 0);
	while (IIC_Wait_Ack(&bh1750_i2c_init_struct));
	
	IIC_Send_Byte(&bh1750_i2c_init_struct, cmd);
	while (IIC_Wait_Ack(&bh1750_i2c_init_struct));
	
	IIC_Stop(&bh1750_i2c_init_struct);
	delay_ms(10);
}

static inline void Start_BH1750(void)
{
	Cmd_Write_BH1750(BH1750_ON);	 // power on
	Cmd_Write_BH1750(BH1750_RSET); // clear
	Cmd_Write_BH1750(BH1750_ONE);
}

static void Read_BH1750(void)
{
	IIC_Start(&bh1750_i2c_init_struct);
	IIC_Send_Byte(&bh1750_i2c_init_struct, BH1750_Addr + 1);
	while (IIC_Wait_Ack(&bh1750_i2c_init_struct));
	BUF[0] = IIC_Read_Byte(&bh1750_i2c_init_struct, 1);
	BUF[1] = IIC_Read_Byte(&bh1750_i2c_init_struct, 0);
	IIC_Stop(&bh1750_i2c_init_struct);
	delay_ms(10);
}

static void Convert_BH1750(void)
{
	Lx_value = BUF[0];
	Lx_value = (Lx_value << 8) + BUF[1];
	Lx_value = (double)Lx_value / 1.2;
}

static void BH1750_Test(void)
{
	uint8_t display[20];
	Start_BH1750();
	delay_ms(400);
	Read_BH1750();
	Convert_BH1750();
	sprintf((char *)display, "%5d Lux", Lx_value); // floatתstring
	oled.show_str(16, 4, (char *)display, 16);
}

static void BH1750_Read(void) {
	Start_BH1750();
	delay_ms(300);
	Read_BH1750();
	Convert_BH1750();
}

static uint16_t BH1750_Get_Lx(void) 
{
	return Lx_value;
}
	


bh1750_i bh1750 = {
	.init = BH1750_Init,
	.test = BH1750_Test,
	.read = BH1750_Read,
	.get = BH1750_Get_Lx,
	.port_name = "P20",
};
