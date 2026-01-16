#ifndef RC522_H
#define RC522_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

//==================== 用户可配置：错误码 ====================
typedef enum {
    RC522_OK           = 0,
    RC522_ERR          = -1,
    RC522_TIMEOUT      = -2,
    RC522_NO_TAG       = -3,
    RC522_COLLISION    = -4,
    RC522_NO_ROOM      = -5,
    RC522_CRC_WRONG    = -6,
    RC522_AUTH_FAIL    = -7,
    RC522_NACK         = -8,
    RC522_INVALID_ARG  = -9
} rc522_status_t;

//==================== ISO14443A 常用命令 ====================
#define RC522_PICC_CMD_REQA          0x26
#define RC522_PICC_CMD_WUPA          0x52
#define RC522_PICC_CMD_CT            0x88
#define RC522_PICC_CMD_SEL_CL1       0x93
#define RC522_PICC_CMD_HLTA          0x50

#define RC522_PICC_CMD_MF_AUTH_KEY_A 0x60
#define RC522_PICC_CMD_MF_AUTH_KEY_B 0x61
#define RC522_PICC_CMD_MF_READ       0x30
#define RC522_PICC_CMD_MF_WRITE      0xA0

#define RC522_MF_ACK                 0x0A
#define RC522_MF_KEY_SIZE            6

//==================== RC522 寄存器（与 Arduino 库一致） ====================
typedef enum {
    // Page 0
    RC522_Reg_Command      = 0x01,
    RC522_Reg_ComIEn       = 0x02,
    RC522_Reg_DivIEn       = 0x03,
    RC522_Reg_ComIrq       = 0x04,
    RC522_Reg_DivIrq       = 0x05,
    RC522_Reg_Error        = 0x06,
    RC522_Reg_Status1      = 0x07,
    RC522_Reg_Status2      = 0x08,
    RC522_Reg_FIFOData     = 0x09,
    RC522_Reg_FIFOLevel    = 0x0A,
    RC522_Reg_Control      = 0x0C,
    RC522_Reg_BitFraming   = 0x0D,
    RC522_Reg_Coll         = 0x0E,

    // Page 1
    RC522_Reg_Mode         = 0x11,
    RC522_Reg_TxMode       = 0x12,
    RC522_Reg_RxMode       = 0x13,
    RC522_Reg_TxControl    = 0x14,
    RC522_Reg_TxASK        = 0x15,
    RC522_Reg_RxSel        = 0x17,
    RC522_Reg_RFCfg        = 0x26,
    RC522_Reg_TMode        = 0x2A,
    RC522_Reg_TPrescaler   = 0x2B,
    RC522_Reg_TReloadH     = 0x2C,
    RC522_Reg_TReloadL     = 0x2D,

    // Page 2
    RC522_Reg_CRCResultH   = 0x21,
    RC522_Reg_CRCResultL   = 0x22,

    // Page 3
    RC522_Reg_Version      = 0x37
} rc522_reg_t;

//==================== PCD 命令 ====================
typedef enum {
    RC522_PCD_Idle       = 0x00,
    RC522_PCD_CalcCRC    = 0x03,
    RC522_PCD_Transceive = 0x0C,
    RC522_PCD_MFAuthent  = 0x0E,
    RC522_PCD_SoftReset  = 0x0F
} rc522_pcd_cmd_t;

//==================== UID / Key ====================
typedef struct {
    uint8_t size;          // 4/7/10
    uint8_t uidByte[10];
    uint8_t sak;
} rc522_uid_t;

typedef struct {
    uint8_t keyByte[RC522_MF_KEY_SIZE];
} rc522_key_t;

//==================== 底层 I2C 抽象 ====================
// 你只要实现这两个函数（或用你自己的 IIC_XXX 包一层）
// 要求：对 RC522 的“寄存器地址 reg”进行读写
typedef int (*rc522_i2c_write_reg_fn)(void *user, uint8_t dev_addr, uint8_t reg, uint8_t value);
typedef int (*rc522_i2c_read_reg_fn) (void *user, uint8_t dev_addr, uint8_t reg, uint8_t *value);

// 可选：延时（不传则库内不主动 delay）
typedef void (*rc522_delay_ms_fn)(void *user, uint32_t ms);

// 可选：日志
typedef void (*rc522_log_fn)(void *user, const char *fmt, ...);

//==================== 设备句柄 ====================
typedef struct {
    uint8_t dev_addr;                 // I2C 7-bit 地址(左对齐/右对齐按你底层实现；推荐传 0x28/0x29 这种“7-bit”)
    void *user;

    rc522_i2c_write_reg_fn i2c_wr;
    rc522_i2c_read_reg_fn  i2c_rd;

    rc522_delay_ms_fn delay_ms;       // 可 NULL
    rc522_log_fn log;                 // 可 NULL
} rc522_t;

//==================== API ====================
rc522_status_t rc522_init(rc522_t *dev);
rc522_status_t rc522_soft_reset(rc522_t *dev);

rc522_status_t rc522_antenna_on(rc522_t *dev);
rc522_status_t rc522_antenna_off(rc522_t *dev);

uint8_t        rc522_read_reg(rc522_t *dev, uint8_t reg);
void           rc522_write_reg(rc522_t *dev, uint8_t reg, uint8_t val);
void           rc522_set_bit_mask(rc522_t *dev, uint8_t reg, uint8_t mask);
void           rc522_clear_bit_mask(rc522_t *dev, uint8_t reg, uint8_t mask);

rc522_status_t rc522_calculate_crc(rc522_t *dev, const uint8_t *data, uint8_t len, uint8_t out[2]);

// ISO14443A 基本流程
rc522_status_t rc522_request_a(rc522_t *dev, uint8_t atqa[2]);     // REQA
rc522_status_t rc522_wakeup_a (rc522_t *dev, uint8_t atqa[2]);     // WUPA
rc522_status_t rc522_anticoll_cl1(rc522_t *dev, uint8_t uid4[4]);  // 只做 CL1(4字节UID)的最常用版本
rc522_status_t rc522_select_cl1(rc522_t *dev, const uint8_t uid4[4], uint8_t *sak);

rc522_status_t rc522_halt_a(rc522_t *dev);

// MIFARE Classic
rc522_status_t rc522_auth_key_a(rc522_t *dev, uint8_t blockAddr, const rc522_key_t *key, const uint8_t uid4[4]);
rc522_status_t rc522_auth_key_b(rc522_t *dev, uint8_t blockAddr, const rc522_key_t *key, const uint8_t uid4[4]);
void           rc522_stop_crypto1(rc522_t *dev);

rc522_status_t rc522_mifare_read_block (rc522_t *dev, uint8_t blockAddr, uint8_t out16[16]);
rc522_status_t rc522_mifare_write_block(rc522_t *dev, uint8_t blockAddr, const uint8_t in16[16]);

// 你 demo 里的“按 sector/block 读写”的便捷封装（M1: sector*4+block）
rc522_status_t rc522_read_sector_block (rc522_t *dev, uint8_t sector, uint8_t block, const rc522_key_t *keyA, char out16_asciiz[17]);
rc522_status_t rc522_write_sector_block(rc522_t *dev, uint8_t sector, uint8_t block, const rc522_key_t *keyA, const char *text);

#ifdef __cplusplus
}
#endif

#endif
