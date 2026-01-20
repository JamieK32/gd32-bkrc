#include "beep.h"
#include "freertos.h"
#include "task.h"
#include "delay.h"

beep_i beep = {
		.state = 0,
    .init = BEEP_Init,
    .control = BEEP_Control,
		.play_music = Beep_Play_Music,
		.play_db = Play_Db,
};

#define BEEP_GAP_MS   120   // 两次鸣叫之间的间隔

static uint8_t  beep_cnt_left = 0;     // 剩余鸣叫次数
static uint8_t  beep_phase = 0;        // 0=空闲 1=鸣叫中 2=间隔中
static uint32_t beep_tick_mark = 0;    // 时间戳
static uint32_t beep_on_ticks = 0;     // 单次鸣叫 tick
static uint32_t beep_gap_ticks = 0;    // 间隔 tick

//根据钢琴键位的频率写出
static const uint16_t MusicalNote[] = {
    0, 262, 277, 294, 311, 330, 349, 370, 392, 415, 440, 466, 494,
    523, 554, 587, 622, 659, 698, 740, 784, 831, 880, 932, 988,
    1047, 1109, 1175, 1245, 1319, 1397, 1480, 1568, 1661, 1760, 1865, 1976
};

void beep_set_count(uint32_t duration_ms, uint8_t n)
{
    if (n == 0) return;

    beep_cnt_left  = n;
    beep_on_ticks  = pdMS_TO_TICKS(duration_ms);
    beep_gap_ticks = pdMS_TO_TICKS(BEEP_GAP_MS);

    beep_phase     = 1;  // 进入鸣叫阶段
    beep_tick_mark = xTaskGetTickCount();

    BEEP_Control(1);     // 立刻开始第一声
}


void beep_tick_process(void)
{
    uint32_t now = xTaskGetTickCount();

    switch (beep_phase)
    {
        case 0: // 空闲
            break;

        case 1: // 正在鸣叫
            if ((now - beep_tick_mark) >= beep_on_ticks)
            {
                BEEP_Control(0);
                beep_cnt_left--;

                if (beep_cnt_left == 0)
                {
                    beep_phase = 0;   // 全部完成
                }
                else
                {
                    beep_phase = 2;   // 进入间隔
                    beep_tick_mark = now;
                }
            }
            break;

        case 2: // 间隔中
            if ((now - beep_tick_mark) >= beep_gap_ticks)
            {
                BEEP_Control(1);      // 下一声
                beep_phase = 1;
                beep_tick_mark = now;
            }
            break;
    }
}


static inline void BEEP_Init(void) {
  rcu_periph_clock_enable(BEEP_RTC);

  gpio_mode_set(BEEP_GPIO_Port, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLDOWN, BEEP_Pin);

  gpio_output_options_set(BEEP_GPIO_Port, GPIO_OTYPE_PP, GPIO_OSPEED_200MHZ,
                          BEEP_Pin);

  gpio_bit_write(BEEP_GPIO_Port, BEEP_Pin, RESET);
}

static inline void BEEP_Control(char state) {
  if (state == 1) {
    gpio_bit_write(BEEP_GPIO_Port, BEEP_Pin, SET);
  } else {
    gpio_bit_write(BEEP_GPIO_Port, BEEP_Pin, RESET);
  }
}


static void Play_Sound(uint16_t frq)
{
	uint32_t time;
	if(frq != 0) {
		time = 500000 / ((uint32_t)frq);		//频率的倒数即是时间，然后此处计算的是T/2的时间，所以为500000
		BEEP_Control(1);
		delay_us(time);
		BEEP_Control(0);
		delay_us(time);
	} else {
			vTaskDelay(pdMS_TO_TICKS(1));
		}
}




static void Beep_Play_Note(uint8_t note, uint8_t duration, int32_t delayT) {
    for (int16_t e = 0; e < ((uint16_t)duration) * MusicalNote[note] / delayT; e++) {
        Play_Sound((uint32_t)MusicalNote[note]);
    }
    vTaskDelay(pdMS_TO_TICKS(40));
}

void Play_Db(uint16_t db) {
    uint32_t start_ticks = xTaskGetTickCount();  // 记录函数开始时的tick
    uint32_t timeout_ticks = 800;                // 超时时间，单位为tick

    while (1) {
        delay_us(db);  // 延迟 db 微秒
        gpio_bit_write(BEEP_GPIO_Port, BEEP_Pin, SET);  // 打开蜂鸣器
        delay_us(db);  // 再次延迟 db 微秒
        gpio_bit_write(BEEP_GPIO_Port, BEEP_Pin, RESET);  // 关闭蜂鸣器

        // 检查是否超时
        if ((xTaskGetTickCount() - start_ticks) >= timeout_ticks) {
            break;  // 超时则退出循环
        }
    }
}

///*********************
// *      播放音乐
// *********************/
static void Beep_Play_Music(const uint8_t *music, uint16_t length) {
    if (music == NULL) return; // 确保长度为偶数
    int delayT = 10;
    for (size_t i = 0; i < length; i += 2) {
        Beep_Play_Note(music[i], music[i + 1], delayT);
    }
}

/*********************
 *      在这里写您任务用到的资源
 *********************/

//天空之城
const uint8_t music_example_1[] = {
    //第1行
    P,    4,    P,    4,    P,    4,    M6,    2,    M7,    2,
    H1,    4+2,M7,    2,    H1,    4,    H3,    4,
    M7,    4+4+4,        M3,    2,    M3,    2,
    //第2行
    M6,    4+2,M5,    2,    M6,    4,    H1,    4,
    M5,    4+4+4,        M3,    4,
    M4,    4+2,M3,    2,    M4,    4,     H1,    4,
    //第3行
    M3,    4+2,P,    2,    H1,    2,    H1,    2,    H1,    2,
    M7,    4+2,M4_,2,    M4_,4,    M7,    4,
    M7,    4+4,P,    4,    M6,    2,    M7,    2,
    //第4行
    H1, 4+2,M7,    2,    H1,    4,    H3,    4,
    M7,    4+4+4,    M3,    2,    M3,    2,
    M6,    4+2,    M5,    2,    M6,    4, H1,    4,
    //第5行
    M5,    4+4+4,    M2,    2,    M3,    2,
    M4,    4,    H1,    2,    M7,    2+2,    H1,    2+4,
    H2,    2,    H2,    2,    H3,    2,    H1,    2+4+4,
    //第6行
    H1,    2,    M7,    2,    M6,    2,    M6,    2,    M7,    4,    M5_,4,
    M6,    4+4+4,    H1,    2,    H2,    2,
    H3,    4+2,H2,    2,    H3,    4,    H5,    4,
    //第7行
    H2,    4+4+4,    M5,    2,    M5,    2,
    H1,    4+2,    M7,    2,    H1,    4,    H3,    4,
    H3,    4+4+4+4,
    //第8行
    M6,    2,    M7,    2, H1,    4,    M7,    4,    H2,    2,    H2,    2,
    H1,    4+2,M5,    2+4+4,
    H4,    4,    H3,    4,    H2,    4,    H1,    4,
    //第9行
    H3,    4+4+4,    H3,    4,
    H6,    4+4,    H5,    4,    H5,    4,
    H3,    2,    H2,    2,    H1,    4+4,    P,    2,    H1,    2,
    //第10行
    H2,    4,    H1,    2,    H2,    2,    H2,    4,    H5,    4,
    H3,    4+4+4,    H3,    4,
    H6,    4+4,    H5,    4+4,
    //第11行
    H3,    2,    H2,    2,    H1,    4+4,    P,    2,    H1,    2,
    H2,    4,    H1,    2,    H2,    2+4,    M7,    4,
    M6,    4+4+4,    M6,    2,    M7,    2,
};

//生日快乐
const uint8_t music_example_2[] = {
    L5, 2, L5, 2, L6, 4, L5, 4, 
    M1, 4, L7, 8,
    L5, 2, L5, 2, L6, 4, L5, 4,
    M2, 4, M1, 8,
    L5, 4, L5, 4, M5, 4, M3, 4,
    M1, 4, L7, 4, L6, 4,
    M4, 4 + 2, M4, 4, M3, 4, M1, 4,
    M2, 4, M1, 8
};

//国赛频率
const uint8_t music_example_3[] = {
    L1, 4, L2, 4, L3, 4, L4, 4, L5, 4, L6, 4, L7, 4
};

const uint8_t music_example_4[] = {
	P,4,M1,2,M1,2,M1,2, M1,2,M2,2,M1,1,M1,1,
	M1,4, P,3, M1,1,M1,2,M1,2,M1,2,M2,1,M3,1,
	M3,4,P,3,L5,1,M3,2,M3,2,M4,2,M3,2,
	M2,4+4+4+4,
	P,4,M1,2,M1,2,M1,2,M1,2,M2,2,M1,1,M1,1,
	M1,4,P,3,M5,1,M1,2,M1,2,M1,2,M2,1,M3,1,
	M3,4,P,3,L5,1,M5,2,M5,2,M5,2,M1,2,
	M2,4+4,P,4,M3,2,M2,2,
	
	M1,4+4,P,2,M1,2,M3,2,M5,
	M6,4+4,P,4,M6,2,M5,2,
	M5,4,M5,2,M5,4,M2,2,M3,2,
	M2,4+4,P,4,M3,2,M2,2,
	M1,4+4,P,2,M1,2,M3,2,M5,2,
	M6,4+4,P,4,M6,2,M5,2,
	M5,4,M5,2,M5,4+2,M2,2,M3,2,
	M2,4+4,P,4,M3,2,M2,2,
	M1,4+4,P,2,M1,2,M3,3,M5,2,
	
	M6,4+4+4,H1,2,M6,2,
	M5,4+2,M5,2,M5,4,H1,2,H2,2,
	H2,4,P,2,M1,2,H1,2,M7,2,M6,2,M7,2,
	H1,4,H1,2,H1,2,H1,2,M6,2,M7,2,M7,2,
	M6,4,M6,2,M6,2,M6,2,M5,2,M3,1,M2,1,M1,2,
	M3,2,M5,2,P,2,M5,2,M5,2,M2,2,M3,2,M3,2
};

const uint8_t music_example_5[] = {
	P, 2, M1, 1, M2, 1, L6, 2, M1, 2, M1, 4, P, 2, M1, 2, M1, 4,
	P, 2, M2, 2, M2, 4, P, 2, L5, 2, M1, 2, L7, 2,
	P, 2, M1, 2, M1, 4, P, 2, M1, 2, M1, 4, P, 2, M3, 2, M3, 4,
	P, 2, L5, 2, M1, 2, M2, 2, M1, 6, P, 2, M1, 2, M1, 2, M2, 1, M1, 1,
	M1, 6, P, 2, M1, 2, M1, 2, M2, 2, M3, 2, M6, 2, M7, 2, H1, 2, P, 2, M6, 2, M6, 2, M5, 2, 
	M5, 2, M3, 2, M1, 2, M2, 2, P, 2
};

const uint8_t music_example_6[] = {
	M6, 2, H1, 2, M7, 1, H2, 2, M6, 1, M6, 1, H1, 2, M7, 4,
	M6, 2, H1, 2, M7, 1, H2, 2, M6, 1, M6, 1, H1, 2, M7, 4,
	M6, 2, H1, 2, M7, 1, H2, 2, M6, 1, M6, 1, H1, 2, M7, 4,
	M6, 2, H1, 2, M7, 1, H2, 3,
	L6, 2, M3, 2, H1, 4, L4, 1, M1, 2, M6, 4,
	L5, 2, M2, 2, M7, 4, L6, 2, M3, 2, H1, 4,
	L6, 2, M3, 2, H1, 4, L4, 2, M1, 2, 
	L5, 2, M2, 2, M7, 4,
	M6, 2, H1, 2, M7, 1, H2, 2, M6, 1, M6, 1, H1, 2, M7, 4,
	M6, 2, H1, 2, M7, 1, H2, 2, M6, 1, M6, 1, H1, 2, M7, 4,
	M6, 2, H1, 2, M7, 1, H2, 2, M6, 1, M6, 1, H1, 2, M7, 4,
	M6, 2, H1, 2, M7, 1, H2, 2, M6, 1, M6, 1, H1, 2, M7, 4,
	L6, 2, L3, 2, M1, 4, L4, 2, L1, 2, L6, 2,
	L5, 1, L2, 1, L7, 4, L6, 2, L3, 2, M1, 4,
	L6, 2, L3, 2, M1, 4, L4, 2, L1, 2, L6, 4,
	L5, 2, L2, 2, L7, 4, L1, 2, L5, 2, M1, 4
};

const uint8_t music_example_7[] = {
	L5, 2, M3, 2, M2, 2, M1, 2,
	L5, 6, L5, 1, L5, 1,
	L5, 2, M3, 2, M2, 2, M1, 2,
	L6, 6, L6, 2, 
	L6, 2, M4, 2, M3, 2, M2, 2,
	L7, 6, L5, 1, L5, 1,
	M5, 2, M5, 2, M4, 2, M5, 2,
	M3, 4, M1, 2, P, 2, 
	L5, 2, M3, 2, M2, 2, M1, 2, 
	L5, 6, 
	L5, 2, M3, 2, M2, 2, M1, 2,
	L6, 6, L6, 2, M4, 2, M3, 2, M2, 2,
	M5, 2, M5, 2, M5, 2, M5, 2, 
	M6, 2, M5, 2, M4, 2, M2, 2, 
	M1, 6,
	M3, 2, M3, 2, M3, 4, 
	M3, 2, M3, 2, M3, 4, 
	M3, 2, M5, 2, M1, 3, M2, 1,
	M3, 6, 
	M1, 2, M1, 2, M1, 4, 
	M1, 2, M1, 2, M1, 4,
	M1, 2, M3, 2, M1, 3, M1, 3, M2, 1,
	M1, 6
};

/*********************
 *      音乐文件的大小信息
 *********************/
size_t music_example_1_size = sizeof(music_example_1) / sizeof(music_example_1[0]);
size_t music_example_2_size = sizeof(music_example_2) / sizeof(music_example_2[0]);
size_t music_example_3_size = sizeof(music_example_3) / sizeof(music_example_3[0]);
size_t music_example_4_size = sizeof(music_example_4) / sizeof(music_example_4[0]);
size_t music_example_5_size = sizeof(music_example_5) / sizeof(music_example_5[0]);
size_t music_example_6_size = sizeof(music_example_6) / sizeof(music_example_6[0]);
size_t music_example_7_size = sizeof(music_example_7) / sizeof(music_example_7[0]);
