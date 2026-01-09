#ifndef __BH1750_H
#define __BH1750_H

#define BH1750_Addr 0x46
#define BH1750_ON 0x01
#define BH1750_CON 0x10
#define BH1750_ONE 0x20
#define BH1750_RSET 0x07

#include "gd32f4xx.h"

typedef struct bh1750_i {
  void (*init)(void);
  uint16_t (*get)(void);
  void (*test)(void);
	void (*read)(void);
	const char port_name[6];
} bh1750_i;

extern bh1750_i bh1750;

#endif
