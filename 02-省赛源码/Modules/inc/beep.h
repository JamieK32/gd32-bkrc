#ifndef __BEEP_H__
#define __BEEP_H__

#include "gd32f4xx.h"

#define BEEP_RTC RCU_GPIOC
#define BEEP_GPIO_Port GPIOC
#define BEEP_Pin GPIO_PIN_12

enum {
    P = 0, L1, L1_, L2, L2_, L3, L4, L4_, L5, L5_,
    L6, L6_, L7, M1, M1_, M2, M2_, M3, M4, M4_,
    M5, M5_, M6, M6_, M7, H1, H1_, H2, H2_, H3,
    H4, H4_, H5, H5_, H6, H6_, H7
};

static void BEEP_Init(void);
static void BEEP_Control(char state);
static void Play_Db(uint16_t db);
static void Beep_Play_Music(const uint8_t *music, uint16_t length);

void beep_set_count(uint32_t duration_ms, uint8_t state);
void beep_tick_process(void);

typedef struct beep_i {
	uint8_t state;
  void (*init)(void);
  void (*control)(char);
	void (*play_music)(const uint8_t *, uint16_t);
  void (*play_db)(uint16_t);
} beep_i;

extern beep_i beep;
extern const uint8_t music_example_1[];
extern size_t music_example_1_size;
extern const uint8_t music_example_2[];
extern size_t music_example_2_size;
extern const uint8_t music_example_3[];
extern size_t music_example_3_size;
extern const uint8_t music_example_4[];
extern size_t music_example_4_size;
extern const uint8_t music_example_5[];
extern size_t music_example_5_size;
extern const uint8_t music_example_6[];
extern size_t music_example_6_size;
extern const uint8_t music_example_7[];
extern size_t music_example_7_size;
#endif
