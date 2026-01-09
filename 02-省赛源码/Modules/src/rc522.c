#include "rc522.h"
#include "iic.h"
#include "oled.h"
#include "stdio.h"
#include "delay.h"
#include <string.h>

//P21端口

static unsigned char CT[2];//卡类型
static unsigned char SN[4]; //卡号
unsigned char READ_RFID[DATA_LEN+2]= {0};
static unsigned char KEY[6]= {0xff,0xff,0xff,0xff,0xff,0xff};//密钥

IIC_Config rc522_i2c_config;

rc522_i rc522 = {
    .init = InitRc522,
		.read = RC522_Read,
		.write = RC522_Write,
		.port_name = "P21",
};

/**************************************************************
 *功  能：	RC522 芯片初始化
 *参  数: 	无
 *返回值: 	无
 **************************************************************/
void InitRc522(void) {
  rc522_i2c_config.scl_pin = GPIO_PIN_6;
  rc522_i2c_config.scl_port = GPIOA;
  rc522_i2c_config.scl_rtc = RCU_GPIOA;
  rc522_i2c_config.sda_pin = GPIO_PIN_7 ;
  rc522_i2c_config.sda_port = GPIOA;
  rc522_i2c_config.sda_rtc = RCU_GPIOA;
  IIC_Init(&rc522_i2c_config);
  delay_ms(100);
  PcdReset();
  PcdAntennaOff();
  delay_ms(2);
  PcdAntennaOn();
  M500PcdConfigISOType('A');
}

void Reset_RC522(void) {
  PcdReset();
  PcdAntennaOff();
  delay_ms(2);
  PcdAntennaOn();
}
/**************************************************************
 *功  能：	寻卡
 *参  数: 	req_code[IN]:寻卡方式
 *		 	0x52 = 寻感应区内所有符合14443A标准的卡
 *		 	0x26 = 寻未进入休眠状态的卡
 *		 	pTagType[OUT]：卡片类型代码
 *		 	0x4400 = Mifare_UltraLight
 *		 	0x0400 = Mifare_One(S50)
 *		 	0x0200 = Mifare_One(S70)
 *		 	0x0800 = Mifare_Pro(X)
 *		 	0x4403 = Mifare_DESFire
 *返回值: 	成功返回MI_OK
 **************************************************************/
char PcdRequest(uint8_t req_code, uint8_t *pTagType) {
  char status;
  uint8_t unLen;
  uint8_t ucComMF522Buf[MAXRLEN];

  ClearBitMask(Status2Reg, 0x08);
  WriteRawRC(BitFramingReg, 0x07);
  SetBitMask(TxControlReg, 0x03);

  ucComMF522Buf[0] = req_code;

  status = PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 1, ucComMF522Buf, &unLen);

  if ((status == MI_OK) && (unLen == 0x10)) {
    *pTagType = ucComMF522Buf[0];
    *(pTagType + 1) = ucComMF522Buf[1];
  } else {
    status = MI_ERR;
  }

  return status;
}

/**************************************************************
 *功  能：	防冲撞
 *参  数: 	pSnr[OUT]:卡片序列号，4字节
 *返回值: 	成功返回MI_OK
 **************************************************************/
char PcdAnticoll(uint8_t *pSnr) {
  char status;
  uint8_t i, snr_check = 0;
  uint8_t unLen;
  uint8_t ucComMF522Buf[MAXRLEN];

  ClearBitMask(Status2Reg, 0x08);
  WriteRawRC(BitFramingReg, 0x00);
  ClearBitMask(CollReg, 0x80);

  ucComMF522Buf[0] = PICC_ANTICOLL1;
  ucComMF522Buf[1] = 0x20;

  status = PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 2, ucComMF522Buf, &unLen);

  if (status == MI_OK) {
    for (i = 0; i < 4; i++) {
      *(pSnr + i) = ucComMF522Buf[i];
      snr_check ^= ucComMF522Buf[i];
    }
    if (snr_check != ucComMF522Buf[i]) {
      status = MI_ERR;
    }
  }

  SetBitMask(CollReg, 0x80);
  return status;
}

/**************************************************************
 *功  能：	选定卡片
 *参  数: 	pSnr[IN]:卡片序列号，4字节
 *返回值: 	成功返回MI_OK
 **************************************************************/
char PcdSelect(uint8_t *pSnr) {
  char status;
  uint8_t i;
  uint8_t unLen;
  uint8_t ucComMF522Buf[MAXRLEN];

  ucComMF522Buf[0] = PICC_ANTICOLL1;
  ucComMF522Buf[1] = 0x70;
  ucComMF522Buf[6] = 0;
  for (i = 0; i < 4; i++) {
    ucComMF522Buf[i + 2] = *(pSnr + i);
    ucComMF522Buf[6] ^= *(pSnr + i);
  }
  CalulateCRC(ucComMF522Buf, 7, &ucComMF522Buf[7]);

  ClearBitMask(Status2Reg, 0x08);

  status = PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 9, ucComMF522Buf, &unLen);

  if ((status == MI_OK) && (unLen == 0x18)) {
    status = MI_OK;
  } else {
    status = MI_ERR;
  }

  return status;
}

/**************************************************************
 *功  能：	验证卡片密码
 *参  数:	auth_mode[IN]: 密码验证模式 0x60 = 验证A密钥 0x61 = 验证B密钥
 *		 	addr[IN]：块地址   pKey[IN]：密码
 *pSnr[IN]：卡片序列号，4字节 返回值: 	成功返回MI_OK
 **************************************************************/
char PcdAuthState(uint8_t auth_mode, uint8_t addr, uint8_t *pKey,
                  uint8_t *pSnr) {
  char status;
  uint8_t unLen;
  uint8_t ucComMF522Buf[MAXRLEN];

  ucComMF522Buf[0] = auth_mode;
  ucComMF522Buf[1] = addr;
  //    for (i=0; i<6; i++)
  //    {    ucComMF522Buf[i+2] = *(pKey+i);   }
  //    for (i=0; i<6; i++)
  //    {    ucComMF522Buf[i+8] = *(pSnr+i);   }
  memcpy(&ucComMF522Buf[2], pKey, 6);
  memcpy(&ucComMF522Buf[8], pSnr, 4);

  status = PcdComMF522(PCD_AUTHENT, ucComMF522Buf, 12, ucComMF522Buf, &unLen);
  if ((status != MI_OK) || (!(ReadRawRC(Status2Reg) & 0x08))) {
    status = MI_ERR;
  }

  return status;
}

/**************************************************************
 *功  能：	读取M1卡一块数据
 *参  数: 	addr[IN]：块地址 pData[OUT]：读出的数据，16字节
 *返回值: 	成功返回MI_OK
 **************************************************************/
char PcdRead(uint8_t addr, uint8_t *p) {
  char status;
  uint8_t unLen;
  uint8_t i, ucComMF522Buf[MAXRLEN];

  ucComMF522Buf[0] = PICC_READ;
  ucComMF522Buf[1] = addr;
  CalulateCRC(ucComMF522Buf, 2, &ucComMF522Buf[2]);

  status = PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 4, ucComMF522Buf, &unLen);
  if ((status == MI_OK) && (unLen == 0x90))
  //   {   memcpy(p , ucComMF522Buf, 16);   }
  {
    for (i = 0; i < 16; i++) {
      *(p + i) = ucComMF522Buf[i];
    }
  } else {
    status = MI_ERR;
  }

  return status;
}

/**************************************************************
 *功  能：	写数据到M1卡一块
 *参  数: 	addr[IN]：块地址  pData[IN]：写入的数据，16字节
 *返回值: 	成功返回MI_OK
 **************************************************************/
char PcdWrite(uint8_t addr, uint8_t *p) {
  char status;
  uint8_t unLen;
  uint8_t i, ucComMF522Buf[MAXRLEN];

  ucComMF522Buf[0] = PICC_WRITE;
  ucComMF522Buf[1] = addr;
  CalulateCRC(ucComMF522Buf, 2, &ucComMF522Buf[2]);

  status = PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 4, ucComMF522Buf, &unLen);

  if ((status != MI_OK) || (unLen != 4) ||
      ((ucComMF522Buf[0] & 0x0F) != 0x0A)) {
    status = MI_ERR;
  }

  if (status == MI_OK) {
    // memcpy(ucComMF522Buf, p , 16);
    for (i = 0; i < 16; i++) {
      ucComMF522Buf[i] = *(p + i);
    }
    CalulateCRC(ucComMF522Buf, 16, &ucComMF522Buf[16]);

    status =
        PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 18, ucComMF522Buf, &unLen);
    if ((status != MI_OK) || (unLen != 4) ||
        ((ucComMF522Buf[0] & 0x0F) != 0x0A)) {
      status = MI_ERR;
    }
  }

  return status;
}

/**************************************************************
 *功  能：	命令卡片进入休眠状态
 *参  数: 	无
 *返回值: 	成功返回MI_OK
 **************************************************************/
char PcdHalt(void) {
  uint8_t status;
  uint8_t unLen;
  uint8_t ucComMF522Buf[MAXRLEN];

  ucComMF522Buf[0] = PICC_HALT;
  ucComMF522Buf[1] = 0;
  CalulateCRC(ucComMF522Buf, 2, &ucComMF522Buf[2]);

  status = PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 4, ucComMF522Buf, &unLen);

  return status;
}

/**************************************************************
 *功  能：	用MF522计算CRC16函数
 *参  数: 	无
 *返回值: 	无
 **************************************************************/
void CalulateCRC(uint8_t *pIn, uint8_t len, uint8_t *pOut) {
  uint8_t i, n;
  ClearBitMask(DivIrqReg, 0x04);
  WriteRawRC(CommandReg, PCD_IDLE);
  SetBitMask(FIFOLevelReg, 0x80);
  for (i = 0; i < len; i++) {
    WriteRawRC(FIFODataReg, *(pIn + i));
  }
  WriteRawRC(CommandReg, PCD_CALCCRC);
  i = 0xFF;
  do {
    n = ReadRawRC(DivIrqReg);
    i--;
  } while ((i != 0) && !(n & 0x04));
  pOut[0] = ReadRawRC(CRCResultRegL);
  pOut[1] = ReadRawRC(CRCResultRegM);
}

/**************************************************************
 *功  能：	复位RC522
 *参  数: 	无
 *返回值: 	成功返回MI_OK
 **************************************************************/
char PcdReset(void) {

  delay_us(10);
  WriteRawRC(CommandReg, PCD_RESETPHASE);
  WriteRawRC(CommandReg, PCD_RESETPHASE);
  delay_us(10);

  // 和Mifare卡通讯，CRC初始值0x6363
  WriteRawRC(ModeReg, 0x3D);
  WriteRawRC(TReloadRegL, 30);
  WriteRawRC(TReloadRegH, 0);
  WriteRawRC(TModeReg, 0x8D);
  WriteRawRC(TPrescalerReg, 0x3E);

  // 必须要
  WriteRawRC(TxAutoReg, 0x40);

  return MI_OK;
}
//////////////////////////////////////////////////////////////////////
// 设置RC632的工作方式
//////////////////////////////////////////////////////////////////////
char M500PcdConfigISOType(uint8_t type) {
  // ISO14443_A
  if (type == 'A') {
    ClearBitMask(Status2Reg, 0x08);
    // 3F
    WriteRawRC(ModeReg, 0x3D);
    // 84
    WriteRawRC(RxSelReg, 0x86);
    // 4F
    WriteRawRC(RFCfgReg, 0x7F);
    // tmoLength);// TReloadVal = 'h6a =tmoLength(dec)
    WriteRawRC(TReloadRegL, 30);
    WriteRawRC(TReloadRegH, 0);
    WriteRawRC(TModeReg, 0x8D);
    WriteRawRC(TPrescalerReg, 0x3E);
    delay_us(1000);
    PcdAntennaOn();
  } else {
    return 1;
  }

  return MI_OK;
}

/**************************************************************
 *功  能：	置RC522寄存器位
 *参  数: 	reg[IN]:寄存器地址  mask[IN]:置位值
 *返回值: 	无
 **************************************************************/
void SetBitMask(uint8_t reg, uint8_t mask) {
  char tmp = 0x0;
  tmp = ReadRawRC(reg);
  // set bit mask
  WriteRawRC(reg, tmp | mask);
}

/**************************************************************
 *功  能：	清RC522寄存器位
 *参  数: 	reg[IN]:寄存器地址  mask[IN]:清位值
 *返回值: 	无
 **************************************************************/
void ClearBitMask(uint8_t reg, uint8_t mask) {
  char tmp = 0x0;
  tmp = ReadRawRC(reg);
  // clear bit mask
  WriteRawRC(reg, tmp & ~mask);
}

/**************************************************************
 *功  能：	通过RC522和ISO14443卡通讯
 *参  数: 	Command[IN]:RC522命令字
 *		 	pInData[IN]:通过RC522发送到卡片的数据
 *		 	InLenByte[IN]:发送数据的字节长度
 *		 	pOutData[OUT]:接收到的卡片返回数据
 *		 	*pOutLenBit[OUT]:返回数据的位长度
 *返回值: 	无
 **************************************************************/
char PcdComMF522(uint8_t Command, uint8_t *pIn, uint8_t InLenByte,
                 uint8_t *pOut, uint8_t *pOutLenBit) {
  char status = MI_ERR;
  uint8_t irqEn = 0x00;
  uint8_t waitFor = 0x00;
  uint8_t lastBits;
  uint8_t n;
  uint16_t i;
  switch (Command) {
  case PCD_AUTHENT:
    irqEn = 0x12;
    waitFor = 0x10;
    break;
  case PCD_TRANSCEIVE:
    irqEn = 0x77;
    waitFor = 0x30;
    break;
  default:
    break;
  }

  WriteRawRC(ComIEnReg, irqEn | 0x80);
  // 清所有中断位
  ClearBitMask(ComIrqReg, 0x80);
  WriteRawRC(CommandReg, PCD_IDLE);
  // 清FIFO缓存
  SetBitMask(FIFOLevelReg, 0x80);

  for (i = 0; i < InLenByte; i++) {
    WriteRawRC(FIFODataReg, pIn[i]);
  }
  WriteRawRC(CommandReg, Command);
  //   	 n = ReadRawRC(CommandReg);

  if (Command == PCD_TRANSCEIVE) {
    // 开始传送
    SetBitMask(BitFramingReg, 0x80);
  }

  // i = 600;//根据时钟频率调整，操作M1卡最大等待时间25ms
  i = 2000;
  do {
    n = ReadRawRC(ComIrqReg);
    i--;
  } while ((i != 0) && !(n & 0x01) && !(n & waitFor));
  ClearBitMask(BitFramingReg, 0x80);

  if (i != 0) {
    if (!(ReadRawRC(ErrorReg) & 0x1B)) {
      status = MI_OK;
      if (n & irqEn & 0x01) {
        status = MI_NOTAGERR;
      }
      if (Command == PCD_TRANSCEIVE) {
        n = ReadRawRC(FIFOLevelReg);
        lastBits = ReadRawRC(ControlReg) & 0x07;
        if (lastBits) {
          *pOutLenBit = (n - 1) * 8 + lastBits;
        } else {
          *pOutLenBit = n * 8;
        }
        if (n == 0) {
          n = 1;
        }
        if (n > MAXRLEN) {
          n = MAXRLEN;
        }
        for (i = 0; i < n; i++) {
          pOut[i] = ReadRawRC(FIFODataReg);
        }
      }
    } else {
      status = MI_ERR;
    }
  }

  // stop timer now
  SetBitMask(ControlReg, 0x80);
  WriteRawRC(CommandReg, PCD_IDLE);
  return status;
}

/**************************************************************
 *功  能：	开启天线（每次启动或关闭天险发射之间应至少有1ms的间隔）
 *参  数: 	无
 *返回值: 	无
 **************************************************************/
void PcdAntennaOn(void) {
  uint8_t i;
  i = ReadRawRC(TxControlReg);
  if (!(i & 0x03)) {
    SetBitMask(TxControlReg, 0x03);
  }
}

/**************************************************************
 *功  能：	关闭天线
 *参  数: 	无
 *返回值: 	无
 **************************************************************/
void PcdAntennaOff(void) { ClearBitMask(TxControlReg, 0x03); }

/////////////////////////////////////////////////////////////////////
// 功    能：读RC632寄存器
// 参数说明：Address[IN]:寄存器地址
// 返    回：读出的值
/////////////////////////////////////////////////////////////////////
uint8_t ReadRawRC(uint8_t Address) {
  uint8_t ucResult = 0;
  ucResult = RC522_RD_Reg(SLA_ADDR, Address);
  // 返回收到的数据
  return ucResult;
}

/////////////////////////////////////////////////////////////////////
// 功    能：写RC632寄存器
// 参数说明：Address[IN]:寄存器地址
//           value[IN]:写入的值
/////////////////////////////////////////////////////////////////////
void WriteRawRC(uint8_t Address, uint8_t value) {
  RC522_WR_Reg(SLA_ADDR, Address, value);
}

/**************************************************************
 *功  能：	读寄存器
 *参  数: 	addr:寄存器地址
 *返回值: 	读到的值
 **************************************************************/
uint8_t RC522_RD_Reg(uint8_t RCsla, uint8_t addr) {
  uint8_t temp = 0;
  IIC_Start(&rc522_i2c_config);
  // 发送写器件指令
  IIC_Send_Byte(&rc522_i2c_config, RCsla);
  temp = IIC_Wait_Ack(&rc522_i2c_config);
  // 发送寄存器地址
  IIC_Send_Byte(&rc522_i2c_config, addr);
  temp = IIC_Wait_Ack(&rc522_i2c_config);
  // 重新启动
  IIC_Start(&rc522_i2c_config);
  // 发送读器件指令
  IIC_Send_Byte(&rc522_i2c_config, RCsla + 1);
  temp = IIC_Wait_Ack(&rc522_i2c_config);
  // 读取一个字节,不继续再读,发送NAK
  temp = IIC_Read_Byte(&rc522_i2c_config, 0);
  // 产生一个停止条件
  IIC_Stop(&rc522_i2c_config);
  // 返回读到的值
  return temp;
}

// 写寄存器
// addr:寄存器地址
// val:要写入的值
// 返回值:无
void RC522_WR_Reg(uint8_t RCsla, uint8_t addr, uint8_t val) {
  IIC_Start(&rc522_i2c_config);
  // 发送写器件指令
  IIC_Send_Byte(&rc522_i2c_config, RCsla);
  IIC_Wait_Ack(&rc522_i2c_config);
  // 发送寄存器地址
  IIC_Send_Byte(&rc522_i2c_config, addr);
  IIC_Wait_Ack(&rc522_i2c_config);
  // 发送值
  IIC_Send_Byte(&rc522_i2c_config, val);
  IIC_Wait_Ack(&rc522_i2c_config);
  // 产生一个停止条件
  IIC_Stop(&rc522_i2c_config);
}

// 等待卡离开
void WaitCardOff(void) {
  unsigned char status, TagType[2];

  while (1) {
    status = PcdRequest(REQ_ALL, TagType);
    if (status) {
      status = PcdRequest(REQ_ALL, TagType);
      if (status) {
        status = PcdRequest(REQ_ALL, TagType);
        if (status) {
          return;
        }
      }
    }
    delay_ms(10);
  }
}

uint16_t read_count = 0;
uint16_t write_count = 0;

// RC522读取数据
uint8_t RC522_Read(unsigned char sector, unsigned char block, char* data) {
    if (rc522.log == NULL) {
        LOG_E("RC522_LOG is NULL");
        return 0;
    }

    unsigned char status;
    unsigned char blockAddress = sector * 4 + block;
    unsigned char keyAddress   = sector * 4 + 3;

    status = PcdRequest(PICC_REQALL, CT); // 检测卡片
    if (status != MI_OK) {
        read_count++;
        rc522.log("Card detection failed (read_fail_count=%u)", (unsigned)read_count);
        return 0;
    }

    // 防冲突
    vTaskDelay(pdMS_TO_TICKS(200));
    rc522.log("Card detected successfully");

    status = PcdAnticoll(SN);
    if (status != MI_OK) {
        read_count++;
        rc522.log("Anti-collision failed (read_fail_count=%u)", (unsigned)read_count);
        return 0;
    }

    rc522.log("Anti-collision successful");

    status = PcdSelect(SN); // 选中卡片
    if (status != MI_OK) {
        read_count++;
        rc522.log("Card selection failed (read_fail_count=%u)", (unsigned)read_count);
        return 0;
    }

    rc522.log("Card selected successfully");

    status = PcdAuthState(0x60, keyAddress, KEY, SN); // 鉴权
    if (status != MI_OK) {
        read_count++;
        rc522.log("Authentication failed (read_fail_count=%u)", (unsigned)read_count);
        return 0;
    }

    memset(READ_RFID, 0, 16); // 初始化读取存储区内容

    status = PcdRead(blockAddress, READ_RFID);
    if (status != MI_OK) {
        read_count++;
        rc522.log("Read failed (read_fail_count=%u)", (unsigned)read_count);
        return 0;
    }

    rc522.log("Read successful. Data: %s", READ_RFID);

    if (data != NULL) {
        strncpy(data, (const char *)READ_RFID, 16);
        data[15] = '\0';
    }

    if (rc522.read_success != NULL) {
        rc522.read_success(READ_RFID);
    }

    return 1;
}

uint8_t RC522_Write(unsigned char sector, unsigned char block, const char* data, ...) {
    if (rc522.log == NULL) {
        LOG_E("RC522_LOG is NULL");
        return 0;
    }

    if (data == NULL) {
        write_count++;
        rc522.log("Data buffer is NULL (write_fail_count=%u)", (unsigned)write_count);
        return 0;
    }

    unsigned char status;
    unsigned char blockAddress = sector * 4 + block;
    unsigned char keyAddress   = sector * 4 + 3;

    status = PcdRequest(PICC_REQALL, CT); // 检测卡片
    if (status != MI_OK) {
        write_count++;
        rc522.log("Card detection failed (write_fail_count=%u)", (unsigned)write_count);
        return 0;
    }

    // 防冲突
    vTaskDelay(pdMS_TO_TICKS(200));
    rc522.log("Card detected successfully");

    status = PcdAnticoll(SN);
    if (status != MI_OK) {
        write_count++;
        rc522.log("Anti-collision failed (write_fail_count=%u)", (unsigned)write_count);
        return 0;
    }

    rc522.log("Anti-collision successful");

    status = PcdSelect(SN); // 选中卡片
    if (status != MI_OK) {
        write_count++;
        rc522.log("Card selection failed (write_fail_count=%u)", (unsigned)write_count);
        return 0;
    }

    rc522.log("Card selected successfully");

    status = PcdAuthState(0x60, keyAddress, KEY, SN); // 鉴权
    if (status != MI_OK) {
        write_count++;
        rc522.log("Authentication failed (write_fail_count=%u)", (unsigned)write_count);
        return 0;
    }

    char formattedData[DATA_LEN];
    va_list args;
    va_start(args, data);
    int len = vsnprintf(formattedData, DATA_LEN, data, args);
    va_end(args);

    if (len < 0 || len >= DATA_LEN) {
        write_count++;
        rc522.log("Formatted data is too long or invalid (write_fail_count=%u)", (unsigned)write_count);
        return 0;
    }

    status = PcdWrite(blockAddress, (uint8_t*)formattedData); // 写入数据
    if (status != MI_OK) {
        write_count++;
        rc522.log("Write failed (write_fail_count=%u)", (unsigned)write_count);
        return 0;
    }

    rc522.log("Write successful. Data written: %s", formattedData);
    return 1;
}
