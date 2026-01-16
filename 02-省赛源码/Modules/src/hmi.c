#include "hmi.h"
#include "oled.h"
#include "usart.h"
#include "gd32f4xx_it.h"

USART_Config lcd_usart_config;

//连接P18端口
#define MAX_BUFFER_SIZE 256

#define HMI_PRINT_PERIOD_MS 300


hmi_i hmi = {
    .init = usart_lcd_init,
    .send_string = HMI_send_string,
		.send_to_slider = HMI_send_to_slider,
		.printf = HMI_printf,
		.send_cmd = HMI_send_cmd,
		.get_widget_val = HMI_get_widget_val,
		.get_wigit_string = HMI_get_wight_string,
		.send_to_curve = HMI_send_curve,
		.printf_throttled = HMI_printf_throttled,
};

static void usart_lcd_init(void) {
  lcd_usart_config.af_x = GPIO_AF_7;
  lcd_usart_config.baud = 9600;
  lcd_usart_config.nvic_irq = USART2_IRQn;
  lcd_usart_config.rx_pin = GPIO_PIN_11;
  lcd_usart_config.rx_port = GPIOC;
  lcd_usart_config.rx_rtc = RCU_GPIOC;
  lcd_usart_config.tx_pin = GPIO_PIN_10;
  lcd_usart_config.tx_port = GPIOC;
  lcd_usart_config.tx_rtc = RCU_GPIOC;
  lcd_usart_config.usartx_rtc = RCU_USART2;
  lcd_usart_config.usart_x = USART2;
  USART_Init(&lcd_usart_config);
}

static void HMI_send_string(char *name, char *showdata) {
  char buffer[MAX_BUFFER_SIZE];
  int len = snprintf(buffer, sizeof(buffer), "%s=\"%s\"\xff\xff\xff", name, showdata);
  USART_Send_Buff(&lcd_usart_config, (uint8_t *)buffer, len);
}

void HMI_printf(char *name, const char *format, ...) {
    char showdata[MAX_BUFFER_SIZE];
    va_list args;
    // 初始化可变参数列表
    va_start(args, format);
    // 使用 vsnprintf 将格式化字符串输出到 showdata
    vsnprintf(showdata, sizeof(showdata), format, args);
    // 结束可变参数处理
    va_end(args);
    // 调用 HMI_send_string 发送格式化后的数据
    HMI_send_string(name, showdata);
}

void HMI_printf_throttled(char *name, const char *format, ...)
{
    static TickType_t last_tick = 0;

    TickType_t now = xTaskGetTickCount();
    if ((now - last_tick) < pdMS_TO_TICKS(HMI_PRINT_PERIOD_MS)) {
        return;   // 未到 500ms，直接丢弃
    }
    last_tick = now;

    char showdata[MAX_BUFFER_SIZE];

    va_list args;
    va_start(args, format);
    vsnprintf(showdata, sizeof(showdata), format, args);
    va_end(args);

    HMI_send_string(name, showdata);
}


static void HMI_send_to_slider(char *name, int num) {
  char buffer[MAX_BUFFER_SIZE];
  int len = snprintf(buffer, sizeof(buffer), "%s=%d\xff\xff\xff", name, num);
  USART_Send_Buff(&lcd_usart_config, (uint8_t *)buffer, len);
}

static void HMI_send_cmd(char *cmd) {
  char buffer[MAX_BUFFER_SIZE];
  int len = snprintf(buffer, sizeof(buffer), "%s\xff\xff\xff", cmd);
  USART_Send_Buff(&lcd_usart_config, (uint8_t *)buffer, len);
}

// 用于归一化 val 的映射函数，使用传入的 min 和 max
uint8_t map_val_to_255(uint16_t val, uint16_t min_val, uint16_t max_val) {
    if (max_val == min_val) {
        // 如果 max 和 min 相等，避免除以 0，直接返回 0 或 255
        return 0;
    }
    // 线性映射到 [0, 255]
    return (uint8_t)((255.0 * (val - min_val)) / (max_val - min_val));
}

int map_val_to_360(float val, float min_val, float max_val) {
    if (max_val == min_val) {
        // 如果 max 和 min 相等，避免除以 0，直接返回 0
        return 0;
    }
    // 线性映射到 [0, 360]
    return (int)((360.0f * (val - min_val)) / (max_val - min_val));
}

static void HMI_send_curve(const char *curve_name, uint8_t channel, uint16_t val, uint16_t min_val, uint16_t max_val) {
		if (val >= max_val) val = max_val;
		if (val <= min_val) val = min_val;
    char buffer[MAX_BUFFER_SIZE];

    // 将 val 映射到 [0, 255] 的范围
    uint8_t scaled_val = map_val_to_255(val, min_val, max_val);

    // 构造指令并发送
    for (int i = 0; i < 10; i++) {
        int len = snprintf(buffer, sizeof(buffer), "add %s.id,%d,%d\xff\xff\xff", curve_name, channel, scaled_val);
        USART_Send_Buff(&lcd_usart_config, (uint8_t *)buffer, len);  // 假设你已有 USART_Send_Buff 函数实现
    }
}

static uint16_t HMI_get_widget_val(uint8_t *screen_data, uint16_t screen_data_size, const char *widget_name) {
	LcdData *lcd_data = parse_lcd_number_data(screen_data, screen_data_size);
	if (!lcd_data) return 0;
	uint8_t ret = 0;
	if (strcmp(lcd_data->name, widget_name) == 0) {
		ret = lcd_data->val;
	}
	free_lcd_data(lcd_data);
	return ret;
}

static void HMI_get_wight_string(uint8_t *screen_data, uint16_t screen_data_size, const char *widget_name, char *target_buffer) {
    if (!screen_data || screen_data_size == 0 || !widget_name || !target_buffer) {
        *target_buffer = '\0'; // 如果输入数据非法，确保目标缓冲区为空字符串
        return;
    }

    // 调用 `parse_lcd_string_data` 来解析 `screen_data`
    LcdData *lcd_data = parse_lcd_string_data(screen_data, screen_data_size);
    if (!lcd_data) {
        *target_buffer = '\0'; // 如果解析失败，返回空字符串
        return;
    }

    // 比较解析出来的名字是否与 `widget_name` 匹配
    if (strcmp(lcd_data->name, widget_name) == 0) {
        // 如果匹配，则将 `str_data` 的内容复制到目标缓冲区
        strcpy(target_buffer, lcd_data->str_data);
    } else {
        // 如果不匹配，将目标缓冲区置为空字符串
        *target_buffer = '\0';
    }

    // 释放动态分配的 `lcd_data`
    free_lcd_data(lcd_data);
}

void free_lcd_data(LcdData *lcd_data) {
  free(lcd_data->name);
	free(lcd_data->str_data);
  free(lcd_data);
}

char *my_strdup(const char *src) {
  size_t len = strlen(src) + 1;
  char *dst = malloc(len);
  if (dst) {
    memcpy(dst, src, len);
  }
  return dst;
}

LcdData *parse_lcd_number_data(uint8_t *data, uint16_t data_size) {
    if (!data || data_size == 0) {  // 检查输入数据是否有效
        return NULL;
    }

    LcdData *lcd_data = malloc(sizeof(LcdData));
    if (!lcd_data) {
        return NULL;  // 如果分配失败，直接返回 NULL
    }

    char name[256] = {0};
    uint16_t val = 0;
    uint8_t namesize = 0;
    uint8_t equalFlag = 0;

    for (int i = 0; i < data_size; i++) {
        if (data[i] == '=') {
            if (equalFlag == 1) {  // 如果等号出现两次，解析失败
                free(lcd_data);  // 如有已有分配，释放内存
                return NULL;
            }
            equalFlag = 1;
            name[namesize] = '\0';
            continue;
        }

        if (equalFlag == 0) {  // 还在解析 name
            if (namesize < 255) {  // 确保不会越界
                name[namesize++] = data[i];
            } else {
                free(lcd_data);  // 输入 name 超过长度限制，则返回 NULL
                return NULL;
            }
        } else {  // 开始解析 val
            if (data[i] < '0' || data[i] > '9') {  // 检查非数字字符
                free(lcd_data);
                return NULL;  // 格式错误：val 包含非法字符
            }
            val = val * 10 + (data[i] - '0');
        }
    }

    // 如果没有等号，或者 name 是空字符串，则解析失败
    if (equalFlag == 0 || namesize == 0) {
        free(lcd_data);
        return NULL;
    }

    // 如果解析成功，填充 lcd_data 结构体
    lcd_data->name = my_strdup(name);  // 动态分配 name
    if (!lcd_data->name) {  // 如果 strdup 失败
        free(lcd_data);
        return NULL;
    }
    lcd_data->val = val;  // 解析的值

    return lcd_data;
}

LcdData *parse_lcd_string_data(uint8_t *data, uint16_t data_size) {
    if (!data || data_size == 0) { // 检查输入是否合法
        return NULL;
    }

    LcdData *lcd_data = malloc(sizeof(LcdData));
    if (!lcd_data) { // 如果内存分配失败
        return NULL;
    }

    char name[256] = {0};
    char string_data[256] = {0};
    uint8_t namesize = 0;
    uint8_t datasize = 0;
    uint8_t equalFlag = 0;

    for (int i = 0; i < data_size; i++) {
        if (data[i] == '=') { // 遇到 '=' 开始分隔 `name` 和 `string_data`
            if (equalFlag == 1) { // 如果已经有一个等号，格式错误
                free(lcd_data);
                return NULL;
            }
            equalFlag = 1;
            name[namesize] = '\0'; // 终止 `name` 字符串
            continue;
        }

        if (equalFlag == 0) { // 解析 `name`
            if (namesize < 255) { // 确保不会越界
                name[namesize++] = data[i];
            } else { // 如果 `name` 太长，释放内存并返回 NULL
                free(lcd_data);
                return NULL;
            }
        } else { // 解析 `string_data`
            if (datasize < 255) { // 确保不会越界
                string_data[datasize++] = data[i];
            } else { // 如果 `string_data` 太长，释放内存并返回 NULL
                free(lcd_data);
                return NULL;
            }
        }
    }

    // 如果没有等号或者 `name` 是空字符串，则解析失败
    if (equalFlag == 0 || namesize == 0) {
        free(lcd_data);
        return NULL;
    }

    string_data[datasize] = '\0'; // 终止 `string_data` 字符串

    lcd_data->name = my_strdup(name); // 动态分配 `name`
    if (!lcd_data->name) { // 如果内存分配失败
        free(lcd_data);
        return NULL;
    }

    lcd_data->str_data = my_strdup(string_data); // 动态分配 `str_data`
    if (!lcd_data->str_data) { // 如果内存分配失败
        free(lcd_data->name); // 释放之前分配的 `name`
        free(lcd_data);
        return NULL;
    }

    return lcd_data; // 返回解析成功的 `LcdData` 结构体
}
