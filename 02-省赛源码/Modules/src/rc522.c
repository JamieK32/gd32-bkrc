#include "rc522.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#define RC522_MAX_FRAME_LEN 64

//------------------ 内部工具 ------------------
static void logf(rc522_t *d, const char *fmt, ...) {
    if (!d || !d->log) return;
    va_list ap;
    va_start(ap, fmt);
    d->log(d->user, fmt, ap); // ⚠️ 如果你 log 回调不支持 va_list，就把这个函数删掉，直接在外面 printf
    va_end(ap);
}

// 为了兼容“log(fmt,...)”这种回调，这里不强行用 va_list。
// 你更推荐把 dev->log 定义成 printf 风格（你 demo 已经是这样），那就别用上面的 logf。

static int check_dev(rc522_t *dev) {
    return dev && dev->i2c_wr && dev->i2c_rd;
}

uint8_t rc522_read_reg(rc522_t *dev, uint8_t reg) {
    uint8_t v = 0;
    if (!check_dev(dev)) return 0;
    dev->i2c_rd(dev->user, dev->dev_addr, reg, &v);
    return v;
}

void rc522_write_reg(rc522_t *dev, uint8_t reg, uint8_t val) {
    if (!check_dev(dev)) return;
    dev->i2c_wr(dev->user, dev->dev_addr, reg, val);
}

void rc522_set_bit_mask(rc522_t *dev, uint8_t reg, uint8_t mask) {
    uint8_t tmp = rc522_read_reg(dev, reg);
    rc522_write_reg(dev, reg, (uint8_t)(tmp | mask));
}

void rc522_clear_bit_mask(rc522_t *dev, uint8_t reg, uint8_t mask) {
    uint8_t tmp = rc522_read_reg(dev, reg);
    rc522_write_reg(dev, reg, (uint8_t)(tmp & (uint8_t)(~mask)));
}

//------------------ 核心：与 PICC 通信（等价 Arduino PCD_CommunicateWithPICC） ------------------
static rc522_status_t rc522_transceive(
    rc522_t *dev,
    uint8_t command,
    const uint8_t *sendData, uint8_t sendLen,
    uint8_t *backData, uint8_t *backLenBytes,
    uint8_t *validBits,    // in/out, last byte valid bits
    uint8_t rxAlign,       // 0..7
    int checkCrc
) {
    if (!check_dev(dev) || !sendData || sendLen == 0) return RC522_INVALID_ARG;

    // BitFramingReg: RxAlign[6:4] + TxLastBits[2:0]
    uint8_t txLastBits = validBits ? (*validBits) : 0;
    uint8_t bitFraming = (uint8_t)((rxAlign << 4) | (txLastBits & 0x07));

    // Stop any command, clear irq, flush fifo
    rc522_write_reg(dev, RC522_Reg_Command, RC522_PCD_Idle);
    rc522_write_reg(dev, RC522_Reg_ComIrq, 0x7F);
    rc522_set_bit_mask(dev, RC522_Reg_FIFOLevel, 0x80);

    // Write FIFO
    for (uint8_t i = 0; i < sendLen; i++) {
        rc522_write_reg(dev, RC522_Reg_FIFOData, sendData[i]);
    }
    rc522_write_reg(dev, RC522_Reg_BitFraming, bitFraming);
    rc522_write_reg(dev, RC522_Reg_Command, command);

    if (command == RC522_PCD_Transceive) {
        rc522_set_bit_mask(dev, RC522_Reg_BitFraming, 0x80); // StartSend
    }

    // Wait
    // waitIRq: RxIRq + IdleIRq => 0x30
    // timer irq bit => 0x01
    const uint8_t waitIRq = 0x30;
    uint16_t i = 2000;
    while (1) {
        uint8_t n = rc522_read_reg(dev, RC522_Reg_ComIrq);
        if (n & waitIRq) break;
        if (n & 0x01) return RC522_TIMEOUT;
        if (--i == 0) return RC522_TIMEOUT;
    }

    // Check errors
    uint8_t err = rc522_read_reg(dev, RC522_Reg_Error);
    // BufferOvfl(0x10) ParityErr(0x02) ProtocolErr(0x01) => 0x13
    if (err & 0x13) return RC522_ERR;
    // Collision
    if (err & 0x08) return RC522_COLLISION;

    // Read back
    if (backData && backLenBytes) {
        uint8_t n = rc522_read_reg(dev, RC522_Reg_FIFOLevel);
        if (n > *backLenBytes) return RC522_NO_ROOM;
        *backLenBytes = n;

        for (uint8_t k = 0; k < n; k++) {
            backData[k] = rc522_read_reg(dev, RC522_Reg_FIFOData);
        }

        uint8_t _validBits = rc522_read_reg(dev, RC522_Reg_Control) & 0x07;
        if (validBits) *validBits = _validBits;
    }

    // CRC check
    if (checkCrc && backData && backLenBytes) {
        uint8_t nbytes = *backLenBytes;
        uint8_t vb = validBits ? *validBits : 0;

        if (nbytes == 1 && vb == 4) return RC522_NACK;
        if (nbytes < 2 || vb != 0) return RC522_CRC_WRONG;

        uint8_t calc[2];
        rc522_status_t st = rc522_calculate_crc(dev, backData, (uint8_t)(nbytes - 2), calc);
        if (st != RC522_OK) return st;

        if (backData[nbytes - 2] != calc[0] || backData[nbytes - 1] != calc[1]) return RC522_CRC_WRONG;
    }

    return RC522_OK;
}

rc522_status_t rc522_calculate_crc(rc522_t *dev, const uint8_t *data, uint8_t len, uint8_t out[2]) {
    if (!check_dev(dev) || !data || len == 0 || !out) return RC522_INVALID_ARG;

    rc522_write_reg(dev, RC522_Reg_Command, RC522_PCD_Idle);
    rc522_write_reg(dev, RC522_Reg_DivIrq, 0x04);          // clear CRCIRq
    rc522_set_bit_mask(dev, RC522_Reg_FIFOLevel, 0x80);    // flush
    for (uint8_t i = 0; i < len; i++) rc522_write_reg(dev, RC522_Reg_FIFOData, data[i]);
    rc522_write_reg(dev, RC522_Reg_Command, RC522_PCD_CalcCRC);

    uint16_t i = 5000;
    while (1) {
        uint8_t n = rc522_read_reg(dev, RC522_Reg_DivIrq);
        if (n & 0x04) break;
        if (--i == 0) return RC522_TIMEOUT;
    }

    rc522_write_reg(dev, RC522_Reg_Command, RC522_PCD_Idle);
    out[0] = rc522_read_reg(dev, RC522_Reg_CRCResultL);
    out[1] = rc522_read_reg(dev, RC522_Reg_CRCResultH);
    return RC522_OK;
}

//------------------ 初始化/复位/天线 ------------------
rc522_status_t rc522_soft_reset(rc522_t *dev) {
    if (!check_dev(dev)) return RC522_INVALID_ARG;
    rc522_write_reg(dev, RC522_Reg_Command, RC522_PCD_SoftReset);

    // 等待 PowerDown 位清除（CommandReg bit4）
    // datasheet 没明确时间，保守一点
    if (dev->delay_ms) dev->delay_ms(dev->user, 50);

    uint16_t guard = 1000;
    while ((rc522_read_reg(dev, RC522_Reg_Command) & (1u << 4)) && guard--) { }
    return (guard == 0) ? RC522_TIMEOUT : RC522_OK;
}

rc522_status_t rc522_antenna_on(rc522_t *dev) {
    if (!check_dev(dev)) return RC522_INVALID_ARG;
    uint8_t v = rc522_read_reg(dev, RC522_Reg_TxControl);
    if ((v & 0x03) != 0x03) rc522_write_reg(dev, RC522_Reg_TxControl, (uint8_t)(v | 0x03));
    return RC522_OK;
}

rc522_status_t rc522_antenna_off(rc522_t *dev) {
    if (!check_dev(dev)) return RC522_INVALID_ARG;
    rc522_clear_bit_mask(dev, RC522_Reg_TxControl, 0x03);
    return RC522_OK;
}

rc522_status_t rc522_init(rc522_t *dev) {
    if (!check_dev(dev)) return RC522_INVALID_ARG;

    rc522_status_t st = rc522_soft_reset(dev);
    if (st != RC522_OK) return st;

    // Timer / CRC preset（与 Arduino 库一致）
    rc522_write_reg(dev, RC522_Reg_TMode,       0x80);
    rc522_write_reg(dev, RC522_Reg_TPrescaler,  0xA9);
    rc522_write_reg(dev, RC522_Reg_TReloadH,    0x03);
    rc522_write_reg(dev, RC522_Reg_TReloadL,    0xE8);

    rc522_write_reg(dev, RC522_Reg_TxASK, 0x40);
    rc522_write_reg(dev, RC522_Reg_Mode,  0x3D);

    return rc522_antenna_on(dev);
}

//------------------ ISO14443A：REQA/WUPA ------------------
static rc522_status_t reqa_or_wupa(rc522_t *dev, uint8_t cmd, uint8_t atqa[2]) {
    if (!check_dev(dev) || !atqa) return RC522_INVALID_ARG;

    rc522_clear_bit_mask(dev, RC522_Reg_Coll, 0x80);
    uint8_t validBits = 7;

    uint8_t backLen = 2;
    rc522_status_t st = rc522_transceive(dev, RC522_PCD_Transceive, &cmd, 1, atqa, &backLen, &validBits, 0, 0);
    if (st != RC522_OK) return st;

    if (backLen != 2 || validBits != 0) return RC522_ERR;
    return RC522_OK;
}

rc522_status_t rc522_request_a(rc522_t *dev, uint8_t atqa[2]) { return reqa_or_wupa(dev, RC522_PICC_CMD_REQA, atqa); }
rc522_status_t rc522_wakeup_a (rc522_t *dev, uint8_t atqa[2]) { return reqa_or_wupa(dev, RC522_PICC_CMD_WUPA, atqa); }

//------------------ 抗冲突 CL1（最常用的 4字节UID） ------------------
rc522_status_t rc522_anticoll_cl1(rc522_t *dev, uint8_t uid4[4]) {
    if (!check_dev(dev) || !uid4) return RC522_INVALID_ARG;

    rc522_clear_bit_mask(dev, RC522_Reg_Status2, 0x08);
    rc522_write_reg(dev, RC522_Reg_BitFraming, 0x00);
    rc522_clear_bit_mask(dev, RC522_Reg_Coll, 0x80);

    uint8_t buf[2] = { RC522_PICC_CMD_SEL_CL1, 0x20 };
    uint8_t back[5] = {0};
    uint8_t backLen = sizeof(back);
    uint8_t validBits = 0;

    rc522_status_t st = rc522_transceive(dev, RC522_PCD_Transceive, buf, 2, back, &backLen, &validBits, 0, 0);
    if (st != RC522_OK) return st;
    if (backLen != 5) return RC522_ERR;

    uint8_t bcc = back[0] ^ back[1] ^ back[2] ^ back[3];
    if (bcc != back[4]) return RC522_ERR;

    memcpy(uid4, back, 4);
    rc522_set_bit_mask(dev, RC522_Reg_Coll, 0x80);
    return RC522_OK;
}

//------------------ 选择 CL1 ------------------
rc522_status_t rc522_select_cl1(rc522_t *dev, const uint8_t uid4[4], uint8_t *sak) {
    if (!check_dev(dev) || !uid4) return RC522_INVALID_ARG;

    uint8_t buf[9];
    buf[0] = RC522_PICC_CMD_SEL_CL1;
    buf[1] = 0x70;
    buf[2] = uid4[0];
    buf[3] = uid4[1];
    buf[4] = uid4[2];
    buf[5] = uid4[3];
    buf[6] = (uint8_t)(uid4[0] ^ uid4[1] ^ uid4[2] ^ uid4[3]);

    rc522_status_t st = rc522_calculate_crc(dev, buf, 7, &buf[7]);
    if (st != RC522_OK) return st;

    uint8_t back[3] = {0};
    uint8_t backLen = 3;
    uint8_t validBits = 0;

    st = rc522_transceive(dev, RC522_PCD_Transceive, buf, 9, back, &backLen, &validBits, 0, 1);
    if (st != RC522_OK) return st;

    if (backLen != 3 || validBits != 0) return RC522_ERR;
    if (sak) *sak = back[0];
    return RC522_OK;
}

//------------------ HALT ------------------
rc522_status_t rc522_halt_a(rc522_t *dev) {
    if (!check_dev(dev)) return RC522_INVALID_ARG;
    uint8_t buf[4] = { RC522_PICC_CMD_HLTA, 0x00, 0x00, 0x00 };
    rc522_status_t st = rc522_calculate_crc(dev, buf, 2, &buf[2]);
    if (st != RC522_OK) return st;

    // HLTA 按标准：超时算成功
    uint8_t backLen = 0;
    st = rc522_transceive(dev, RC522_PCD_Transceive, buf, 4, NULL, &backLen, NULL, 0, 0);
    return (st == RC522_TIMEOUT) ? RC522_OK : st;
}

//------------------ MIFARE Classic：鉴权/停止加密 ------------------
static rc522_status_t auth_common(rc522_t *dev, uint8_t cmd, uint8_t blockAddr, const rc522_key_t *key, const uint8_t uid4[4]) {
    if (!check_dev(dev) || !key || !uid4) return RC522_INVALID_ARG;

    uint8_t buf[12];
    buf[0] = cmd;
    buf[1] = blockAddr;
    memcpy(&buf[2], key->keyByte, 6);
    memcpy(&buf[8], uid4, 4);

    // waitIRq=IdleIRq(0x10) 的逻辑在 Arduino 里是用 CommunicateWithPICC；
    // 这里复用 transceive 的等待逻辑（ComIrq wait 0x30），实际项目也能跑。
    // 若你想更严格：可改成专门的 auth wait 逻辑（ComIrq wait 0x10）。
    uint8_t backLen = 0;
    rc522_status_t st = rc522_transceive(dev, RC522_PCD_MFAuthent, buf, 12, NULL, &backLen, NULL, 0, 0);
    if (st != RC522_OK) return st;

    // Status2Reg 的 MFCrypto1On(0x08) 必须置位
    if (!(rc522_read_reg(dev, RC522_Reg_Status2) & 0x08)) return RC522_AUTH_FAIL;
    return RC522_OK;
}

rc522_status_t rc522_auth_key_a(rc522_t *dev, uint8_t blockAddr, const rc522_key_t *key, const uint8_t uid4[4]) {
    return auth_common(dev, RC522_PICC_CMD_MF_AUTH_KEY_A, blockAddr, key, uid4);
}
rc522_status_t rc522_auth_key_b(rc522_t *dev, uint8_t blockAddr, const rc522_key_t *key, const uint8_t uid4[4]) {
    return auth_common(dev, RC522_PICC_CMD_MF_AUTH_KEY_B, blockAddr, key, uid4);
}

void rc522_stop_crypto1(rc522_t *dev) {
    if (!check_dev(dev)) return;
    rc522_clear_bit_mask(dev, RC522_Reg_Status2, 0x08);
}

//------------------ MIFARE Read/Write block ------------------
rc522_status_t rc522_mifare_read_block(rc522_t *dev, uint8_t blockAddr, uint8_t out16[16]) {
    if (!check_dev(dev) || !out16) return RC522_INVALID_ARG;

    uint8_t buf[4];
    buf[0] = RC522_PICC_CMD_MF_READ;
    buf[1] = blockAddr;

    rc522_status_t st = rc522_calculate_crc(dev, buf, 2, &buf[2]);
    if (st != RC522_OK) return st;

    uint8_t back[18] = {0};
    uint8_t backLen = sizeof(back);
    st = rc522_transceive(dev, RC522_PCD_Transceive, buf, 4, back, &backLen, NULL, 0, 1);
    if (st != RC522_OK) return st;
    if (backLen < 18) return RC522_ERR;

    memcpy(out16, back, 16);
    return RC522_OK;
}

static rc522_status_t mifare_transceive_ack(rc522_t *dev, const uint8_t *send, uint8_t sendLen, int acceptTimeout) {
    uint8_t cmdBuf[18];
    if (sendLen > 16) return RC522_INVALID_ARG;
    memcpy(cmdBuf, send, sendLen);

    rc522_status_t st = rc522_calculate_crc(dev, cmdBuf, sendLen, &cmdBuf[sendLen]);
    if (st != RC522_OK) return st;
    uint8_t total = (uint8_t)(sendLen + 2);

    uint8_t back[18] = {0};
    uint8_t backLen = 18;
    uint8_t validBits = 0;

    st = rc522_transceive(dev, RC522_PCD_Transceive, cmdBuf, total, back, &backLen, &validBits, 0, 0);
    if (acceptTimeout && st == RC522_TIMEOUT) return RC522_OK;
    if (st != RC522_OK) return st;

    if (backLen != 1 || validBits != 4) return RC522_ERR;
    if ((back[0] & 0x0F) != RC522_MF_ACK) return RC522_NACK;
    return RC522_OK;
}

rc522_status_t rc522_mifare_write_block(rc522_t *dev, uint8_t blockAddr, const uint8_t in16[16]) {
    if (!check_dev(dev) || !in16) return RC522_INVALID_ARG;

    uint8_t cmd[2] = { RC522_PICC_CMD_MF_WRITE, blockAddr };
    rc522_status_t st = mifare_transceive_ack(dev, cmd, 2, 0);
    if (st != RC522_OK) return st;

    st = mifare_transceive_ack(dev, in16, 16, 0);
    return st;
}

//------------------ 你的 demo 级封装：sector/block 读写 ------------------
static rc522_status_t select_one_card_cl1(rc522_t *dev, uint8_t uid4[4], uint8_t atqa[2], uint8_t *sak) {
    rc522_status_t st = rc522_request_a(dev, atqa);
    if (st != RC522_OK) return st;

    st = rc522_anticoll_cl1(dev, uid4);
    if (st != RC522_OK) return st;

    st = rc522_select_cl1(dev, uid4, sak);
    return st;
}

rc522_status_t rc522_read_sector_block(rc522_t *dev, uint8_t sector, uint8_t block, const rc522_key_t *keyA, char out16_asciiz[17]) {
    if (!dev || !keyA || !out16_asciiz) return RC522_INVALID_ARG;

    uint8_t uid4[4], atqa[2], sak = 0;
    rc522_status_t st = select_one_card_cl1(dev, uid4, atqa, &sak);
    if (st != RC522_OK) return st;

    uint8_t blockAddr = (uint8_t)(sector * 4 + block);
    uint8_t keyBlock  = (uint8_t)(sector * 4 + 3);

    st = rc522_auth_key_a(dev, keyBlock, keyA, uid4);
    if (st != RC522_OK) return st;

    uint8_t data16[16];
    st = rc522_mifare_read_block(dev, blockAddr, data16);

    rc522_stop_crypto1(dev);
    rc522_halt_a(dev);

    if (st != RC522_OK) return st;

    memcpy(out16_asciiz, data16, 16);
    out16_asciiz[16] = '\0';
    return RC522_OK;
}

rc522_status_t rc522_write_sector_block(rc522_t *dev, uint8_t sector, uint8_t block, const rc522_key_t *keyA, const char *text) {
    if (!dev || !keyA || !text) return RC522_INVALID_ARG;

    uint8_t uid4[4], atqa[2], sak = 0;
    rc522_status_t st = select_one_card_cl1(dev, uid4, atqa, &sak);
    if (st != RC522_OK) return st;

    uint8_t blockAddr = (uint8_t)(sector * 4 + block);
    uint8_t keyBlock  = (uint8_t)(sector * 4 + 3);

    st = rc522_auth_key_a(dev, keyBlock, keyA, uid4);
    if (st != RC522_OK) return st;

    uint8_t data16[16];
    memset(data16, 0, sizeof(data16));
    // 文本写入（最多 16 字节）
    strncpy((char*)data16, text, 16);

    st = rc522_mifare_write_block(dev, blockAddr, data16);

    rc522_stop_crypto1(dev);
    rc522_halt_a(dev);

    return st;
}
