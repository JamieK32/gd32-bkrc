#include "iic.h"
#include "delay.h"

// ========================= 可调参数 =========================
// 建议先用 5~10us，线长/上拉弱就用更大
#ifndef IIC_DELAY_US
#define IIC_DELAY_US  5
#endif

#ifndef IIC_ACK_TIMEOUT_US
#define IIC_ACK_TIMEOUT_US  300
#endif
// ==========================================================


// ===== GPIO 读写（用赋值，不用 |=，避免对写寄存器做RMW）=====
#define I2C_SCL_LOW(cfg)   (GPIO_BC((cfg)->scl_port)  = (cfg)->scl_pin)
#define I2C_SCL_HIGH(cfg)  (GPIO_BOP((cfg)->scl_port) = (cfg)->scl_pin)
#define I2C_SDA_LOW(cfg)   (GPIO_BC((cfg)->sda_port)  = (cfg)->sda_pin)
#define I2C_SDA_HIGH(cfg)  (GPIO_BOP((cfg)->sda_port) = (cfg)->sda_pin)

#define I2C_SCL_READ(cfg)  (GPIO_ISTAT((cfg)->scl_port) & (cfg)->scl_pin)
#define I2C_SDA_READ(cfg)  (GPIO_ISTAT((cfg)->sda_port) & (cfg)->sda_pin)

static inline void iic_delay_us(uint32_t us) {
	delay_us(us);
}

// clock stretching：释放 SCL 后等它真正变高
static inline void iic_wait_scl_high(IIC_Config *cfg) {
    // 如果从设备拉低SCL，会在这里等待；加个上限防死等
    uint32_t guard = 1000;
    while (!I2C_SCL_READ(cfg) && guard--) { __NOP(); }
}
// ==========================================================

void IIC_Init(IIC_Config *cfg) {
  rcu_periph_clock_enable(cfg->scl_rtc);
  rcu_periph_clock_enable(cfg->sda_rtc);

  // I2C 正确做法：开漏 OD + 上拉（外部4.7k~10k更稳）
  gpio_mode_set(cfg->scl_port, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, cfg->scl_pin);
  gpio_output_options_set(cfg->scl_port, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ, cfg->scl_pin);

  gpio_mode_set(cfg->sda_port, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, cfg->sda_pin);
  gpio_output_options_set(cfg->sda_port, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ, cfg->sda_pin);

  // 释放总线
  I2C_SCL_HIGH(cfg);
  I2C_SDA_HIGH(cfg);
  iic_delay_us(IIC_DELAY_US * 2);
}

void SDA_IN(IIC_Config *cfg) {
  rcu_periph_clock_enable(cfg->sda_rtc);
  gpio_mode_set(cfg->sda_port, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, cfg->sda_pin);
}

void SDA_OUT(IIC_Config *cfg) {
  rcu_periph_clock_enable(cfg->sda_rtc);
  gpio_mode_set(cfg->sda_port, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, cfg->sda_pin);
  gpio_output_options_set(cfg->sda_port, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ, cfg->sda_pin);
  // 释放线（OD写1就是释放）
  I2C_SDA_HIGH(cfg);
}

void IIC_Start(IIC_Config *cfg) {
  SDA_OUT(cfg);

  I2C_SDA_HIGH(cfg);
  I2C_SCL_HIGH(cfg);
  iic_delay_us(IIC_DELAY_US);

  // START: SCL=1 时 SDA: 1->0
  I2C_SDA_LOW(cfg);
  iic_delay_us(IIC_DELAY_US);

  I2C_SCL_LOW(cfg);
  iic_delay_us(IIC_DELAY_US);
}

void IIC_Stop(IIC_Config *cfg) {
  SDA_OUT(cfg);

  I2C_SCL_LOW(cfg);
  I2C_SDA_LOW(cfg);
  iic_delay_us(IIC_DELAY_US);

  // STOP: SCL=1 时 SDA: 0->1
  I2C_SCL_HIGH(cfg);
  iic_wait_scl_high(cfg);
  iic_delay_us(IIC_DELAY_US);

  I2C_SDA_HIGH(cfg);
  iic_delay_us(IIC_DELAY_US);
}

uint8_t IIC_Wait_Ack(IIC_Config *cfg) {
  uint32_t t = 0;

  SDA_IN(cfg);           // 释放 SDA，让从机拉低应答
  iic_delay_us(1);

  I2C_SCL_HIGH(cfg);
  iic_wait_scl_high(cfg);
  iic_delay_us(IIC_DELAY_US);

  while (I2C_SDA_READ(cfg)) {
    t++;
    if (t >= IIC_ACK_TIMEOUT_US) {
      IIC_Stop(cfg);
      return 1; // NACK/超时
    }
    iic_delay_us(1);
  }

  I2C_SCL_LOW(cfg);
  iic_delay_us(IIC_DELAY_US);
  return 0;
}

void IIC_Ack(IIC_Config *cfg) {
  I2C_SCL_LOW(cfg);
  SDA_OUT(cfg);
  I2C_SDA_LOW(cfg);
  iic_delay_us(IIC_DELAY_US);

  I2C_SCL_HIGH(cfg);
  iic_wait_scl_high(cfg);
  iic_delay_us(IIC_DELAY_US);

  I2C_SCL_LOW(cfg);
  I2C_SDA_HIGH(cfg); // 释放
  iic_delay_us(IIC_DELAY_US);
}

void IIC_NAck(IIC_Config *cfg) {
  I2C_SCL_LOW(cfg);
  SDA_OUT(cfg);
  I2C_SDA_HIGH(cfg); // 释放 = 1
  iic_delay_us(IIC_DELAY_US);

  I2C_SCL_HIGH(cfg);
  iic_wait_scl_high(cfg);
  iic_delay_us(IIC_DELAY_US);

  I2C_SCL_LOW(cfg);
  iic_delay_us(IIC_DELAY_US);
}

void IIC_Send_Byte(IIC_Config *cfg, uint8_t txd) {
  SDA_OUT(cfg);
  I2C_SCL_LOW(cfg);
  iic_delay_us(IIC_DELAY_US);

  for (uint8_t t = 0; t < 8; t++) {
    if (txd & 0x80) I2C_SDA_HIGH(cfg);  // OD 写1=释放
    else           I2C_SDA_LOW(cfg);

    txd <<= 1;

    iic_delay_us(IIC_DELAY_US);

    I2C_SCL_HIGH(cfg);
    iic_wait_scl_high(cfg);
    iic_delay_us(IIC_DELAY_US);

    I2C_SCL_LOW(cfg);
    iic_delay_us(IIC_DELAY_US);
  }

  // 发送完一字节后释放 SDA，准备ACK
  I2C_SDA_HIGH(cfg);
}

uint8_t IIC_Read_Byte(IIC_Config *cfg, unsigned char ack) {
  uint8_t receive = 0;
  SDA_IN(cfg);

  for (uint8_t i = 0; i < 8; i++) {
    I2C_SCL_LOW(cfg);
    iic_delay_us(IIC_DELAY_US);

    I2C_SCL_HIGH(cfg);
    iic_wait_scl_high(cfg);
    iic_delay_us(IIC_DELAY_US);

    receive <<= 1;
    if (I2C_SDA_READ(cfg)) receive++;

    iic_delay_us(IIC_DELAY_US);
  }

  I2C_SCL_LOW(cfg);
  iic_delay_us(IIC_DELAY_US);

  if (ack) IIC_Ack(cfg);
  else     IIC_NAck(cfg);

  return receive;
}

// 7-bit DevAddress 通用接口（保留）
void IIC_Master_Transmit(IIC_Config *cfg, uint16_t DevAddress, uint8_t *pData,
                         uint16_t Size) {
  IIC_Start(cfg);
  IIC_Send_Byte(cfg, (uint8_t)((DevAddress << 1) | 0));
  if (IIC_Wait_Ack(cfg)) return;

  while (Size--) {
    IIC_Send_Byte(cfg, *pData++);
    if (IIC_Wait_Ack(cfg)) return;
  }
  IIC_Stop(cfg);
}

uint8_t IIC_Master_Receive(IIC_Config *cfg, uint16_t DevAddress, uint8_t *pData,
                           uint16_t Size) {
  IIC_Start(cfg);
  IIC_Send_Byte(cfg, (uint8_t)((DevAddress << 1) | 1));
  if (IIC_Wait_Ack(cfg)) {
    IIC_Stop(cfg);
    return 1;
  }

  while (Size) {
    *pData = IIC_Read_Byte(cfg, (Size != 1));
    pData++;
    Size--;
  }
  IIC_Stop(cfg);
  return 0;
}

// 总线恢复：SDA 被拉低卡死时，踢 9 个 SCL
void IIC_BusRecover(IIC_Config *cfg) {
  SDA_IN(cfg);            // 释放 SDA
  I2C_SDA_HIGH(cfg);
  SDA_OUT(cfg);           // 确保我们能驱动 SCL
  I2C_SDA_HIGH(cfg);

  // 释放 SCL
  I2C_SCL_HIGH(cfg);
  iic_delay_us(IIC_DELAY_US);

  // 如果 SDA 低，说明总线卡住，输出 9 个时钟
  if (!I2C_SDA_READ(cfg)) {
    for (int i = 0; i < 9; i++) {
      I2C_SCL_LOW(cfg);
      iic_delay_us(IIC_DELAY_US);
      I2C_SCL_HIGH(cfg);
      iic_wait_scl_high(cfg);
      iic_delay_us(IIC_DELAY_US);
    }
  }

  // 发一个 STOP
  IIC_Stop(cfg);
}
