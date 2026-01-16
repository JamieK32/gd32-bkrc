#ifndef __BH1750_H
#define __BH1750_H

#define BH1750_Addr 0x46
#define BH1750_ON 0x01
#define BH1750_CON 0x10
#define BH1750_ONE 0x20
#define BH1750_RSET 0x07

#include "gd32f4xx.h"
#include "stdbool.h"

typedef struct {
    void     (*init)(void);
    void     (*tick)(void);         // 周期调用推进状态机（并自动周期采样）
    bool     (*is_ready)(void);     // 是否有“新数据”可取（取完会自动清）
    uint16_t (*get)(void);          // 获取最新 lux（建议配合 is_ready 使用）
    const char *port_name;
} bh1750_i;

extern bh1750_i bh1750;

#endif
