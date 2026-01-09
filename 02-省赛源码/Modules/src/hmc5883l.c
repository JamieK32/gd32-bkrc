#include "hmc5883l.h"
#include "iic.h"
#include "delay.h"
#include "math.h"

// IIC 配置结构体
IIC_Config i2c_config = {
	.scl_pin = GPIO_PIN_0,
	.scl_port = GPIOC,
	.scl_rtc = RCU_GPIOC,
	.sda_pin = GPIO_PIN_1,
	.sda_port = GPIOC,
	.sda_rtc = RCU_GPIOC,
};

hmc5883l_t hmc5883l = {
    .init = HMC5883L_Init,
    .show_compass = Show_CompassVal,
    .get_angle = HCM5883L_Get_Angle,
};

// HMC5883L 设备地址 (7-bit address)
#define HMC5883L_ADDR 0x1E  // 0x3C >> 1

void HMC5883L_Init(void)
{
    // 初始化 I2C
    IIC_Init(&i2c_config);

    // 设置输出速率 (寄存器 0x00, 值 0x58 表示 75Hz)
    uint8_t config_a[] = {0x00, 0x58};
    IIC_Master_Transmit(&i2c_config, HMC5883L_ADDR, config_a, sizeof(config_a));

    // 设置测量范围 (寄存器 0x01, 值 0x60)
    uint8_t config_b[] = {0x01, 0x60};
    IIC_Master_Transmit(&i2c_config, HMC5883L_ADDR, config_b, sizeof(config_b));

    // 设置连续测量模式 (寄存器 0x02, 值 0x00)
    uint8_t mode[] = {0x02, 0x00};
    IIC_Master_Transmit(&i2c_config, HMC5883L_ADDR, mode, sizeof(mode));
}

void HMC5883L_READ(int16_t *x, int16_t *y)
{
    uint8_t XYZ_Data[6] = {0};
    uint8_t reg_addr = 0x03;  // X 轴数据起始地址

    // 发送寄存器地址
    IIC_Master_Transmit(&i2c_config, HMC5883L_ADDR, &reg_addr, 1);

    // 读取 6 个字节的数据 (X, Z, Y 顺序)
    IIC_Master_Receive(&i2c_config, HMC5883L_ADDR, XYZ_Data, 6);

    // 组合 X 和 Y 数据 (忽略 Z 轴)
    *x = (int16_t)((XYZ_Data[0] << 8) | XYZ_Data[1]);
    *y = (int16_t)((XYZ_Data[4] << 8) | XYZ_Data[5]);
}

void HCM5883L_Init(void)
{
    HMC5883L_Init(); // 磁力计初始化
}

double HCM5883L_Get_Angle(void)
{
    int16_t X_HM, Y_HM;
    double Angle = 0;
    delay_ms(67);  // 等待数据准备 (根据 75Hz 输出速率)
    HMC5883L_READ(&X_HM, &Y_HM);

    Angle = (atan2(Y_HM, X_HM) * (180 / 3.14159265) + 180);

    return Angle;
}

uint8_t Show_CompassVal(void)
{
    float angle = (int)(HCM5883L_Get_Angle()) % 360; // 磁力计

    // 定义方向和对应的角度范围
    const char *directions[] = {
        "北", "东北", "东", "东南", "南", "西南", "西", "西北"
    };

    // 定义对应的角度范围
    const int angle_ranges[] = {
        23, 68, 113, 158, 203, 248, 293, 338, 360
    };

    // 查找方向
    int direction_index = 0;
    for (int i = 0; i < 8; i++)
    {
        if (angle < angle_ranges[i])
        {
            direction_index = i;
            break;
        }
    }
    snprintf((char *)hmc5883l.output_str, sizeof(hmc5883l.output_str), "  角度: %3.1f° 方向:  %s  ", angle, directions[direction_index]);
		return direction_index;
}
