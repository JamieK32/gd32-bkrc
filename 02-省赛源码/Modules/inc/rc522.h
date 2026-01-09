/*
 * rc522.h
 *
 * created: 23/12/2024
 *  author:
 */

#ifndef _RC522_H
#define _RC522_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "stdint.h"
#include "stdarg.h"
#include "gd32f4xx.h"
#include "main.h"
#include "freertos.h"
#include "task.h"

// 根据芯片信号选择设备地址
#define SLA_ADDR 0x5e
    // #define SLA_ADDR 0x50

    /////////////////////////////////////////////////////////////////////
    // MF522命令字
    /////////////////////////////////////////////////////////////////////

#define PCD_IDLE 0x00       // 取消当前命令
#define PCD_AUTHENT 0x0E    // 验证密钥
#define PCD_RECEIVE 0x08    // 接收数据
#define PCD_TRANSMIT 0x04   // 发送数据
#define PCD_TRANSCEIVE 0x0C // 发送并接收数据
#define PCD_RESETPHASE 0x0F // 复位
#define PCD_CALCCRC 0x03    // CRC计算

/////////////////////////////////////////////////////////////////////
// Mifare_One卡片命令字
/////////////////////////////////////////////////////////////////////
#define PICC_REQIDL 0x26    // 寻天线区内未进入休眠状态
#define PICC_REQALL 0x52    // 寻天线区内全部卡
#define PICC_ANTICOLL1 0x93 // 防冲撞
#define PICC_ANTICOLL2 0x95 // 防冲撞
#define PICC_AUTHENT1A 0x60 // 验证A密钥
#define PICC_AUTHENT1B 0x61 // 验证B密钥
#define PICC_READ 0x30      // 读块
#define PICC_WRITE 0xA0     // 写块
#define PICC_DECREMENT 0xC0 // 扣款
#define PICC_INCREMENT 0xC1 // 充值
#define PICC_RESTORE 0xC2   // 调块数据到缓冲区
#define PICC_TRANSFER 0xB0  // 保存缓冲区中数据
#define PICC_HALT 0x50      // 休眠

/////////////////////////////////////////////////////////////////////
// MF522 FIFO长度定义
/////////////////////////////////////////////////////////////////////
#define DEF_FIFO_LENGTH 64 // FIFO size=64byte
#define MAXRLEN 18

/////////////////////////////////////////////////////////////////////
// MF522寄存器定义
/////////////////////////////////////////////////////////////////////
// PAGE 0
#define RFU00 0x00
#define CommandReg 0x01
#define ComIEnReg 0x02
#define DivlEnReg 0x03
#define ComIrqReg 0x04
#define DivIrqReg 0x05
#define ErrorReg 0x06
#define Status1Reg 0x07
#define Status2Reg 0x08
#define FIFODataReg 0x09
#define FIFOLevelReg 0x0A
#define WaterLevelReg 0x0B
#define ControlReg 0x0C
#define BitFramingReg 0x0D
#define CollReg 0x0E
#define RFU0F 0x0F
// PAGE 1
#define RFU10 0x10
#define ModeReg 0x11
#define TxModeReg 0x12
#define RxModeReg 0x13
#define TxControlReg 0x14
#define TxAutoReg 0x15
#define TxSelReg 0x16
#define RxSelReg 0x17
#define RxThresholdReg 0x18
#define DemodReg 0x19
#define RFU1A 0x1A
#define RFU1B 0x1B
#define MifareReg 0x1C
#define RFU1D 0x1D
#define RFU1E 0x1E
#define SerialSpeedReg 0x1F
// PAGE 2
#define RFU20 0x20
#define CRCResultRegM 0x21
#define CRCResultRegL 0x22
#define RFU23 0x23
#define ModWidthReg 0x24
#define RFU25 0x25
#define RFCfgReg 0x26
#define GsNReg 0x27
#define CWGsCfgReg 0x28
#define ModGsCfgReg 0x29
#define TModeReg 0x2A
#define TPrescalerReg 0x2B
#define TReloadRegH 0x2C
#define TReloadRegL 0x2D
#define TCounterValueRegH 0x2E
#define TCounterValueRegL 0x2F
// PAGE 3
#define RFU30 0x30
#define TestSel1Reg 0x31
#define TestSel2Reg 0x32
#define TestPinEnReg 0x33
#define TestPinValueReg 0x34
#define TestBusReg 0x35
#define AutoTestReg 0x36
#define VersionReg 0x37
#define AnalogTestReg 0x38
#define TestDAC1Reg 0x39
#define TestDAC2Reg 0x3A
#define TestADCReg 0x3B
#define RFU3C 0x3C
#define RFU3D 0x3D
#define RFU3E 0x3E
#define RFU3F 0x3F

#define REQ_ALL 0x52
#define KEYA 0x60
#define KEYB 0x61

/////////////////////////////////////////////////////////////////////
// 和MF522通讯时返回的错误代码
/////////////////////////////////////////////////////////////////////
#define MI_OK 0
#define MI_NOTAGERR (1)
#define MI_ERR (2)

#define SHAQU1 0X01
#define KUAI4 0X04
#define KUAI7 0X07
#define REGCARD 0xa1
#define CONSUME 0xa2
#define READCARD 0xa3
#define ADDMONEY 0xa4

//自行添加的常量
#define DATA_LEN 16

typedef struct rc522_i
{
  void (*read_success)(unsigned char *);
	void (*log)(const char *, ...);
	void (*init)(void);
	uint8_t (*read)(unsigned char, unsigned char, char *);
	uint8_t (*write)(unsigned char, unsigned char, const char *, ...);
	const char port_name[4];
} rc522_i;

extern rc522_i rc522;


// 初始化RC522
void InitRc522(void);

// 清除寄存器的特定位
void ClearBitMask(uint8_t reg, uint8_t mask);

// 写入数据到RC522寄存器
void WriteRawRC(uint8_t Address, uint8_t value);

// 设置寄存器的特定位
void SetBitMask(uint8_t reg, uint8_t mask);

// 执行RC522命令
char PcdComMF522(uint8_t Command, uint8_t *pIn, uint8_t InLenByte,
                 uint8_t *pOut, uint8_t *pOutLenBit);

// 计算CRC校验码
void CalulateCRC(uint8_t *pIn, uint8_t len, uint8_t *pOut);

// 读取RC522寄存器的数据
uint8_t ReadRawRC(uint8_t Address);

// 打开RC522的天线
void PcdAntennaOn(void);

// 复位RC522
char PcdReset(void);

// 发送请求命令，检测卡片类型
char PcdRequest(unsigned char req_code, unsigned char *pTagType);

// 打开RC522的天线
void PcdAntennaOn(void);

// 关闭RC522的天线
void PcdAntennaOff(void);

// 配置RC522为ISO14443类型
char M500PcdConfigISOType(unsigned char type);

// 防碰撞检测，获取卡片序列号
char PcdAnticoll(unsigned char *pSnr);

// 选择卡片
char PcdSelect(unsigned char *pSnr);

// 验证卡片密码
char PcdAuthState(unsigned char auth_mode, unsigned char addr,
                  unsigned char *pKey, unsigned char *pSnr);

// 向卡片写数据
char PcdWrite(unsigned char addr, unsigned char *pData);

// 从卡片读数据
char PcdRead(unsigned char addr, unsigned char *pData);

// 让卡片进入休眠状态
char PcdHalt(void);

// 复位RC522
void Reset_RC522(void);

// 读取RC522寄存器（通过I2C/SPI接口）
uint8_t RC522_RD_Reg(uint8_t RCsla, uint8_t addr);

// 写入RC522寄存器（通过I2C/SPI接口）
void RC522_WR_Reg(uint8_t RCsla, uint8_t addr, uint8_t val);

// 等待卡片移出感应区
void WaitCardOff(void);

//重要读写卡片
uint8_t RC522_Read(unsigned char sector, unsigned char block, char* data);
uint8_t RC522_Write(unsigned char sector, unsigned char block, const char* data, ...);

#ifdef __cplusplus
}
#endif

#endif // _RC522_H
