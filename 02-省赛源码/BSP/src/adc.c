#include "adc.h"
#include "delay.h"

// 配置时钟
static void rcu_config(const ADC_Config *config) {
    rcu_periph_clock_enable(config->rcu_gpio);  // 使能 GPIO 时钟
    rcu_periph_clock_enable(config->rcu_adc);   // 使能 ADC 时钟
    adc_clock_config(config->clock_div);        // 配置 ADC 时钟分频
}

// 配置 GPIO
static void gpio_config(const ADC_Config *config) {
    gpio_mode_set(config->gpio_port, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, config->gpio_pin);
}

// 配置 ADC
static void adc_periph_config(const ADC_Config *config) {
    adc_deinit();                          // 复位 ADC
    adc_sync_mode_config(ADC_SYNC_MODE_INDEPENDENT);         // 独立模式
    adc_special_function_config(config->adc_periph, ADC_CONTINUOUS_MODE, DISABLE); // 禁用连续模式
    adc_special_function_config(config->adc_periph, ADC_SCAN_MODE, DISABLE);       // 禁用扫描模式
    adc_data_alignment_config(config->adc_periph, ADC_DATAALIGN_RIGHT);           // 数据右对齐
    adc_channel_length_config(config->adc_periph, ADC_REGULAR_CHANNEL, 1U);       // 单通道

    // 配置触发源（可扩展为参数化）
    adc_external_trigger_source_config(config->adc_periph, ADC_REGULAR_CHANNEL, ADC_EXTTRIG_REGULAR_T0_CH0);
    adc_external_trigger_config(config->adc_periph, ADC_REGULAR_CHANNEL, EXTERNAL_TRIGGER_DISABLE);

    adc_enable(config->adc_periph);  // 使能 ADC
    delay_ms(1U);                   // 等待稳定
    adc_calibration_enable(config->adc_periph);  // 校准
}

// 初始化 ADC
void adc_init(const ADC_Config *config) {
    rcu_config(config);
    gpio_config(config);
    adc_periph_config(config);
}

// 采样 ADC 数据
uint16_t adc_sample(const ADC_Config *config) {
    adc_regular_channel_config(config->adc_periph, 0U, config->adc_channel, config->sample_time); // 配置通道
    adc_software_trigger_enable(config->adc_periph, ADC_REGULAR_CHANNEL);                        // 软件触发

    while (!adc_flag_get(config->adc_periph, ADC_FLAG_EOC));  // 等待转换完成
    adc_flag_clear(config->adc_periph, ADC_FLAG_EOC);         // 清除标志
    return adc_regular_data_read(config->adc_periph);         // 返回采样值
}

