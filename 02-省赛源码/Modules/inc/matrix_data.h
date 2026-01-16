#ifndef MATRIX_DATA_H
#define MATRIX_DATA_H

#include <stdint.h>
#include <stddef.h>


extern uint8_t matrix_data_9[80];
extern uint8_t matrix_data_99[10][4];
extern uint8_t matrix_chinese_data_1[40];

extern uint8_t arrow_right[8];
extern uint8_t arrow_left[8];
extern uint8_t arrow_up[8];
extern uint8_t arrow_down[8];

void reverse_blocks_u8(uint8_t *data, size_t len, size_t block_size);

#define REVERSE_BLOCKS_ARRAY(arr) reverse_blocks_u8((arr), sizeof(arr), 8)

#endif
