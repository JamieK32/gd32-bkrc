#include "c1016.h"
#include "oled.h"
#include "stdio.h"
#include "delay.h"
#include "timer.h"
#include "usart.h"
#include "stdbool.h"
#include "main.h"

//连接P22和P13端口

// USART Configuration
USART_Config c1016_usart_config = {
    .tx_rtc = RCU_GPIOA,
    .rx_rtc = RCU_GPIOA,
    .usartx_rtc = RCU_USART5,
    .tx_port = GPIOA,
    .tx_pin = GPIO_PIN_11,
    .rx_port = GPIOA,
    .rx_pin = GPIO_PIN_12,
    .baud = 115200,
    .usart_x = USART5,
    .af_x = GPIO_AF_8,
    .nvic_irq = USART5_IRQn,
};

/*********************
 *     静态变量
 *********************/
volatile uint8_t c1016_rx_buffer[RX_BUFFER_SIZE];
static uint8_t c1016_ids[C1016_MAX_ID_SIZE] = {0};
static uint8_t c1016_id_number = 1;
static uint16_t delay_time = 300;

/*********************
 *      创建颜色实例
 *********************/
C1016_COLOR_Struct c1016_color = {
    .color_mode = C1016_COLOR_MODE_RESPIRE,        //慢呼吸
    .color_start_value = C1016_COLOR_YELLOW,        //开始颜色蓝色
    .color_end_value = 0,                         //结束颜色白色
    .color_loop = 0,                              //循环
};
 
/*********************
 *      创建指纹模块实例
 *********************/
c1016_t c1016 = {
    .enroll_state = 0,                  //初始化注册状态
    .init = C1016_Init,                 //初始化IO
    .enroll = C1016_Enroll,             //注册
    .identfify = C1016_Identify,        //识别
    .delete_id = C1016_cmd_Delete,      //删除
    .show_format_ids = C1016_Print_Format_Ids, //格式化打印IDS
};



/*********************
 *      以下是C1016自己添加的部分，格式化打印ID
 *********************/
void C1016_Print_Format_Ids(void) {
    if (c1016.show_log == NULL) return;
    char temp_buffer[256];
    uint8_t temp_ind = 0;
    for (int i = 1; i < C1016_MAX_ID_SIZE; i++) {
        if (c1016_ids[i] == 1) {
            // Convert the ID to a string and append to the buffer
            temp_ind += snprintf(&temp_buffer[temp_ind], sizeof(temp_buffer) - temp_ind, "%d ", i);
        }
    }
    // Ensure the buffer is null-terminated
    temp_buffer[temp_ind] = '\0';
    c1016.show_log("%s", temp_buffer);
}


/*********************
 *      以下是C1016的INT引脚部分
 *********************/
void C1016_INT_Init(void) {
  rcu_periph_clock_enable(C1016_RCU);
  gpio_mode_set(C1016_PORT, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, C1016_PIN);
}

int C1016_Read_INT(void) {
	return gpio_input_bit_get(C1016_PORT, C1016_PIN);
}

/*********************
 *      以下是C1016的串口发送接收
 *********************/
/*******************************************************************
 **函数名：C1016_UART_Init
 **函数功能：初始化uart的IO口
 **说明：   UART1_RX:50 -- 数据接收
            UART1_TX:51 -- 数据发送
 *******************************************************************/
void C1016_UART_Init(void) {
	USART_Init(&c1016_usart_config);
}

void C1016_Clear_Rx_Buffer(void) {
    memset((char *)c1016_rx_buffer, 0, 26);
}

void C1016_Init(void) {
    C1016_INT_Init();
    C1016_UART_Init();
		C1016_SetColor(&c1016_color);
    vTaskDelay(pdMS_TO_TICKS(delay_time));
}

//串口发送数据
void C1016_Send_Data(unsigned char *da, unsigned char len)
{
    if (da == NULL || len == 0) return;
		USART_Send_Buff(&c1016_usart_config, da, len);
}

/*********************
 *      以下是C1016的校验计算部分
 *********************/
 
//用二维数组简化写内存
//校验和计算函数
static inline void C1016_Data_Verification(uint8_t *da, uint8_t len)
{
    if (da == NULL || len == 0) return;
    uint32_t lenl, lenh, lensum;
    if (len >= 26)
    {
        lenl = da[0] + da[1] + da[2] + da[3] + da[4] + da[5] + da[6] + da[7] +
               da[8] + da[9] + da[10];
        lenh = da[11] + da[12] + da[13] + da[14] + da[15] + da[16] + da[17] +
               da[18] + da[19] + da[20] + da[21] + da[22] + da[23];
        lensum = (lenl + lenh) % 65536;
        da[24] = lensum % 256;
        da[25] = lensum / 256;
    }
}

/*********************
 *      以下是C1016的底层API部分
 *********************/
 
 /**
 * 删除start到end编号范围内的指纹
 * **/
void C1016_cmd_Delete(uint16_t start, uint16_t end)
{
    if (c1016.show_log != NULL) c1016.show_log("delete id %d - %d", start, end);
    for (int i = start; i <= end; i++) {
        c1016_ids[i] = 0;
    }
    CMD_DEL_CHAR[8] = start & 0xFF;
    CMD_DEL_CHAR[9] = (start >> 8) & 0xFF;
    CMD_DEL_CHAR[10] = end & 0xFF;
    CMD_DEL_CHAR[11] = (end >> 8) & 0xFF;
    C1016_Data_Verification(CMD_DEL_CHAR, sizeof(CMD_DEL_CHAR)); //验证数据
    C1016_Send_Data(CMD_DEL_CHAR, sizeof(CMD_DEL_CHAR));         //发送数据
    vTaskDelay(pdMS_TO_TICKS(100));                    //接收数据
    LOG_HEX(c1016_rx_buffer, 26);                                //显示数据，如果接收到的数据有问题可以参考数据手册
    C1016_Clear_Rx_Buffer();                                     //清空接收缓冲区
}

void C1016_cmd_search(uint16_t start, uint16_t end) {
    CMD_SEARCH[10] = start & 0xFF;
    CMD_SEARCH[11] = (start >> 8) & 0xFF;
    CMD_SEARCH[12] = end & 0xFF;
    CMD_SEARCH[13] = (end >> 8) & 0xFF;
    C1016_Data_Verification(CMD_SEARCH, sizeof(CMD_SEARCH));
    C1016_Send_Data(CMD_SEARCH, sizeof(CMD_SEARCH));
    vTaskDelay(pdMS_TO_TICKS(delay_time));
}

/**
 * 将暂存于ImageBuffer中的指纹图片生成特征模板数据，并保存与指定的RamBuffer中
 * **/
void C1016_cmd_Generate(unsigned int RamBufferx)
{
    CMD_GENERATE[8] = RamBufferx & 0xFF;
    CMD_GENERATE[9] = (RamBufferx >> 8) & 0xFF;
    C1016_Data_Verification(CMD_GENERATE, sizeof(CMD_GENERATE));
    C1016_Send_Data(CMD_GENERATE, sizeof(CMD_GENERATE));
}


/**
 * 用颜色结构体设置灯颜色
 * **/
void C1016_SetColor(C1016_COLOR_Struct *p)
{
    c1016.show_log("set color");
    uint8_t pData1[] = {0x55, 0xaa, 0x00, 0x00, 0x24, 0x00, 0x04, 0x00, 0x03,
                        0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2C, 0x01};
    pData1[8] = p->color_mode;
    pData1[9] = p->color_start_value;
    pData1[10] = p->color_end_value;
    pData1[11] = p->color_loop;
    C1016_Data_Verification(pData1, 26);            //验证数据
    C1016_Send_Data(pData1, 26);                    //发送数据
    vTaskDelay(pdMS_TO_TICKS(delay_time));          //短暂延时
    LOG_HEX(c1016_rx_buffer, 26);                   //显示数据，如果接收到的数据有问题可以参考数据手册
    C1016_Clear_Rx_Buffer();                        //清空接收缓冲区
}

/*********************
 *      以下是C1016的应用层API部分 基于数据手册重构
 *********************/
 
/*********************
 *     重要 录入指纹
 *********************/
void C1016_Enroll(void) {

    unsigned int c1016_enroll_time = 0;

    if (c1016.show_log == NULL) {
        LOG_E("c1016.show_log is NULL");
        return;
    }

    c1016.show_log("Start typing fingerprints");
    c1016.show_log("Checking whether the ID is registered");

    while (1) {
        if (!C1016_Read_INT()) {
            if (c1016_enroll_time >= 5000) {
                c1016.show_log("Enrollment timed out");
                break;
            }
            c1016_enroll_time++;
            vTaskDelay(pdMS_TO_TICKS(1));
            continue;
        }

        if (check_id_registered()) {
            return;
        }

        if (c1016.enroll_state == 1 && capture_fingerprint_image("One", 0)) {
            if (!save_fingerprint_image(0)) return;
        }

        if (c1016.enroll_state == 3 && capture_fingerprint_image("Two", 1)) {
            if (!save_fingerprint_image(1)) return;
        }

        if (c1016.enroll_state == 5 && capture_fingerprint_image("Three", 2)) {
            if (!save_fingerprint_image(2)) return;
        }

        if (c1016.enroll_state == 7 && merge_fingerprint_images()) {
            if (!save_fingerprint_template()) return;
        }

        if (c1016.enroll_state == 9) {
            c1016_ids[c1016_id_number] = 1;
            c1016.show_log("Fingerprint enrollment successful, id =  %d", c1016_id_number++);
            c1016.enroll_state = 0;
            break;
        }
    }
}

/*********************
 *     重要 识别指纹
 *********************/
void C1016_Identify(void) {
    if (c1016.show_log == NULL) {
        LOG_E("c1016.show_log is NULL");
        return;
    }

    c1016.show_log("Starting fingerprint identification...");


    while (C1016_Read_INT() == 0) {
        c1016.show_log("No finger detected");
        vTaskDelay(pdMS_TO_TICKS(delay_time));
    }

    c1016.show_log("Capturing fingerprint image");
    if (!capture_fingerprint_image("Identification", 0)) {
        c1016.show_log("Fingerprint image capture failed");
        return;
    }
    
    if (!save_fingerprint_image(0)) {
        c1016.show_log("save faild");
    }

    c1016.show_log("search fingerprint");
    if (!C1016_search_fingerprint(1, 40)) {
        c1016.show_log("search fingerprint faild");
    }
}

/*********************
 *      查看ID是否注册
 *********************/
bool check_id_registered(void) {
    CMD_GET_STATU[8] = c1016_id_number & 0xFF;
    CMD_GET_STATU[9] = (c1016_id_number >> 8) & 0xFF;
    C1016_Data_Verification(CMD_GET_STATU, sizeof(CMD_GET_STATU));
    C1016_Send_Data(CMD_GET_STATU, sizeof(CMD_GET_STATU));
    vTaskDelay(pdMS_TO_TICKS(delay_time));

    while (1) {
        LOG_HEX(c1016_rx_buffer, 26);
        if (c1016_rx_buffer[4] == 0x46 && c1016_rx_buffer[6] == 0x03) {
            if (c1016_rx_buffer[8] == 0x00) {
                c1016.show_log("The ID is not registered");
                c1016.enroll_state = 1;
                C1016_Clear_Rx_Buffer();
                return false;
            } else {
                c1016.show_log("The ID is registered");
                return true;
            }
        } else {
            c1016.show_log("CMD_GET_STATU RECEIVE DATA ERROR");
            return true;
        }
    }
}

/*********************
 *      捕获指纹图像
 *********************/
bool capture_fingerprint_image(const char* image_stage, int buffer_index) {
    c1016.show_log("Capturing fingerprint image %s", image_stage);
    C1016_Data_Verification(CMD_GET_IMAGE, sizeof(CMD_GET_IMAGE));
    C1016_Send_Data(CMD_GET_IMAGE, sizeof(CMD_GET_IMAGE));
    vTaskDelay(pdMS_TO_TICKS(delay_time));

    while (1) {
        LOG_HEX(c1016_rx_buffer, 26);
        if (c1016_rx_buffer[4] == 0x20 && c1016_rx_buffer[6] == 0x02) {
            if (c1016_rx_buffer[8] == 0x00) {
                c1016.show_log("Fingerprint image %s collection completed", image_stage);
                c1016.enroll_state += 1;
                C1016_Clear_Rx_Buffer();
                return true;
            } else {
                c1016.show_log("Fingerprint image %s collection failed", image_stage);
                return false;
            }
        } else {
            c1016.show_log("CMD_GET_IMAGE RECEIVE DATA ERROR");
            return false;
        }
    }
}

/*********************
 *      保存指纹图像
 *********************/
bool save_fingerprint_image(int buffer_index) {
    c1016.show_log("Saving fingerprint image %d", buffer_index + 1);
    C1016_cmd_Generate(buffer_index);
    vTaskDelay(pdMS_TO_TICKS(delay_time));
    
    while (1) {
        LOG_HEX(c1016_rx_buffer, 26);
        if (c1016_rx_buffer[4] == 0x60 && c1016_rx_buffer[6] == 0x02) {
            if (c1016_rx_buffer[8] == 0x00) {
                c1016.show_log("Save OK");
                c1016.enroll_state += 1;
                C1016_Clear_Rx_Buffer();
                return true;
            } else {
                c1016.show_log("Save Failed");
                return false;
            }
        } else {
            c1016.show_log("SAVE %d ERROR", buffer_index + 1);
            return false;
        }
    }
}

/*********************
 *      合成指纹图像
 *********************/
bool merge_fingerprint_images(void) {
    c1016.show_log("Merging fingerprint images");
    C1016_Data_Verification(CMD_MERGE, sizeof(CMD_MERGE));
    C1016_Send_Data(CMD_MERGE, sizeof(CMD_MERGE));
    vTaskDelay(pdMS_TO_TICKS(delay_time));

    while (1) {
        LOG_HEX(c1016_rx_buffer, 26);
        if (c1016_rx_buffer[4] == 0x61 && c1016_rx_buffer[6] == 0x02) {
            if (c1016_rx_buffer[8] == 0x00) {
                c1016.show_log("Fingerprint images merged successfully");
                c1016.enroll_state += 1;
                C1016_Clear_Rx_Buffer();
                return true;
            } else {
                c1016.show_log("Fingerprint images merge failed");
                return false;
            }
        } else {
            c1016.show_log("CMD_MERGE RECEIVE DATA ERROR");
            return false;
        }
    }
}


/*********************
 *      保存指纹模板
 *********************/
bool save_fingerprint_template(void) {
    c1016.show_log("Saving fingerprint template");
    CMD_STORE_CHAR[8] = c1016_id_number & 0xFF;
    CMD_STORE_CHAR[9] = (c1016_id_number >> 8) & 0xFF;
    C1016_Data_Verification(CMD_STORE_CHAR, sizeof(CMD_STORE_CHAR));
    C1016_Send_Data(CMD_STORE_CHAR, sizeof(CMD_STORE_CHAR));
    vTaskDelay(pdMS_TO_TICKS(delay_time));

    while (1) {
        LOG_HEX(c1016_rx_buffer, 26);
        if (c1016_rx_buffer[4] == 0x40 && c1016_rx_buffer[6] == 0x02) {
            if (c1016_rx_buffer[8] == 0x00) {
                c1016.show_log("Fingerprint template saved successfully");
                c1016.enroll_state += 1;
                C1016_Clear_Rx_Buffer();
                return true;
            } else {
                c1016.show_log("Fingerprint template save failed");
                return false;
            }
        } else {
            c1016.show_log("CMD_STORE_CHAR RECEIVE DATA ERROR");
            return false;
        }
    }
}


/*********************
 *      查找指定ID
 *********************/
bool C1016_search_fingerprint(uint16_t start, uint16_t end) {
    C1016_cmd_search(start, end);
    vTaskDelay(pdMS_TO_TICKS(delay_time));
    while (1) {
        LOG_HEX(c1016_rx_buffer, 26);

        if (c1016_rx_buffer[4] == 0x63 && c1016_rx_buffer[6] == 0x05) {
            c1016.identify_id = ((c1016_rx_buffer[11] << 8) + c1016_rx_buffer[10]);
            c1016.show_log("verified successfully, id = %d", c1016.identify_id);
            return true;
        } else if (c1016_rx_buffer[4] == 0x65) {
            c1016.show_log("Fingerprint verification failed");
            return false;
        } else {
            c1016.show_log("CMD_VERIFY RECEIVE DATA ERROR");
            return false;
        }
    }
}
