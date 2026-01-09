#include "oled.h"
#include "iic.h"
#include "oledfont.h"
#include "stdio.h"
#include "stdarg.h"
#include "delay.h"

IIC_Config oled_i2c_init_struct;

static void OLED_Hardware_Init(void) {
  oled_i2c_init_struct.scl_pin = GPIO_PIN_7;
  oled_i2c_init_struct.scl_port = GPIOB;
  oled_i2c_init_struct.scl_rtc = RCU_GPIOB;
  oled_i2c_init_struct.sda_pin = GPIO_PIN_6;
  oled_i2c_init_struct.sda_port = GPIOB;
  oled_i2c_init_struct.sda_rtc = RCU_GPIOB;
  IIC_Init(&oled_i2c_init_struct);
}

static void OLED_WR_Byte(uint8_t dat, uint8_t cmd_or_data) {
  IIC_Start(&oled_i2c_init_struct);
  IIC_Send_Byte(&oled_i2c_init_struct, I2C_OLED);
  IIC_Wait_Ack(&oled_i2c_init_struct);
  IIC_Send_Byte(&oled_i2c_init_struct, cmd_or_data);
  IIC_Wait_Ack(&oled_i2c_init_struct);
  IIC_Send_Byte(&oled_i2c_init_struct, dat);
  IIC_Wait_Ack(&oled_i2c_init_struct);
  IIC_Stop(&oled_i2c_init_struct);
}

static void OLED_Set_Pos(unsigned char x, unsigned char y) {
  OLED_WR_Byte(0xb0 + y, IIC_OLED_CMD);
  OLED_WR_Byte(((x & 0xf0) >> 4) | 0x10, IIC_OLED_CMD);
  OLED_WR_Byte((x & 0x0f), IIC_OLED_CMD);
}

static void OLED_Clear(void) {
  uint8_t i, n;
  for (i = 0; i < 8; i++) {
    OLED_WR_Byte(0xb0 + i, IIC_OLED_CMD);
    OLED_WR_Byte(0x02, IIC_OLED_CMD);
    OLED_WR_Byte(0x10, IIC_OLED_CMD);
    for (n = 0; n < 128; n++) {
      OLED_WR_Byte(0x00, IIC_OLED_DATA);
    }
  }
}

static void OLED_DrawBMP(unsigned char x0, unsigned char y0, unsigned char x1,
                  unsigned char y1, unsigned char BMP[]) {
  unsigned int j = 0;
  unsigned char x, y;
  if (y1 % 8 == 0) {
    y = y1 / 8;
  } else {
    y = y1 / 8 + 1;
  }
  for (y = y0; y < y1; y++) {
    OLED_Set_Pos(x0, y);
    for (x = x0; x < x1; x++) {
      OLED_WR_Byte(BMP[j++], IIC_OLED_DATA);
    }
  }
}

static void OLED_Init(void) {
  OLED_Hardware_Init();

  delay_ms(100);

  OLED_WR_Byte(0xAE, IIC_OLED_CMD); //--turn off oled panel
  OLED_WR_Byte(0x00, IIC_OLED_CMD); //---set low column address
  OLED_WR_Byte(0x10, IIC_OLED_CMD); //---set high column address
  OLED_WR_Byte(0x40, IIC_OLED_CMD); //--set start line address  Set Mapping RAM
                                    // Display Start Line (0x00~0x3F)
  OLED_WR_Byte(0x81, IIC_OLED_CMD); //--set contrast control register
  OLED_WR_Byte(0xCF, IIC_OLED_CMD); // Set SEG Output Current Brightness
  OLED_WR_Byte(0xA1, IIC_OLED_CMD); //--Set SEG/Column Mapping
  OLED_WR_Byte(0xC8, IIC_OLED_CMD); // Set COM/Row Scan Direction
  OLED_WR_Byte(0xA6, IIC_OLED_CMD); //--set normal display
  OLED_WR_Byte(0xA8, IIC_OLED_CMD); //--set multiplex ratio(1 to 64)
  OLED_WR_Byte(0x3f, IIC_OLED_CMD); //--1/64 duty

  OLED_WR_Byte(0x81, IIC_OLED_CMD);
  OLED_WR_Byte(0xFF, IIC_OLED_CMD);

  OLED_WR_Byte(0xD3, IIC_OLED_CMD); //-set display offset   Shift Mapping RAM
                                    // Counter (0x00~0x3F)
  OLED_WR_Byte(0x00, IIC_OLED_CMD); //-not offset
  OLED_WR_Byte(
      0xd5,
      IIC_OLED_CMD); //--set display clock divide ratio/oscillator frequency
  OLED_WR_Byte(0x80,
               IIC_OLED_CMD); //--set divide ratio, Set Clock as 100 Frames/Sec
  OLED_WR_Byte(0xD9, IIC_OLED_CMD); //--set pre-charge period
  OLED_WR_Byte(
      0xF1, IIC_OLED_CMD); // Set Pre-Charge as 15 Clocks & Discharge as 1 Clock
  OLED_WR_Byte(0xDA, IIC_OLED_CMD); //--set com pins hardware configuration
  OLED_WR_Byte(0x12, IIC_OLED_CMD);
  OLED_WR_Byte(0xDB, IIC_OLED_CMD); //--set vcomh
  OLED_WR_Byte(0x40, IIC_OLED_CMD); // Set VCOM Deselect Level
  OLED_WR_Byte(0x20, IIC_OLED_CMD); //-Set Page Addressing Mode (0x00/0x01/0x02)
  OLED_WR_Byte(0x02, IIC_OLED_CMD); //
  OLED_WR_Byte(0x8D, IIC_OLED_CMD); //--set Charge Pump enable/disable
  OLED_WR_Byte(0x14, IIC_OLED_CMD); //--set(0x10) disable
  OLED_WR_Byte(0xA4, IIC_OLED_CMD); // Disable Entire Display On (0xa4/0xa5)
  OLED_WR_Byte(0xA6, IIC_OLED_CMD); // Disable Inverse Display On (0xa6/a7)
  OLED_WR_Byte(0xAF, IIC_OLED_CMD); //--turn on oled panel

  OLED_WR_Byte(0xAF, IIC_OLED_CMD); /*display ON*/
  OLED_Clear();
  OLED_Set_Pos(0, 0);
}

static void OLED_ShowChar(uint16_t x, uint16_t y, uint8_t num, uint8_t wsize) {
  uint8_t pos = 0, t = 0;

  if (wsize != 16) {
    wsize = 16;
  }

  num = num - ' ';

  for (pos = 0; pos < wsize / 8; pos++) {
    OLED_Set_Pos(x, y + pos);
    for (t = 0; t < wsize / 2; t++) {
      OLED_WR_Byte(asc2_1608[num][pos * 8 + t], IIC_OLED_DATA);
    }
  }
}

static void OLED_ShowString(uint16_t x, uint16_t y, char *p, uint8_t wsize) {
  while ((*p <= '~') && (*p >= ' ')) {
    if (x > (128 - wsize / 2)) {
      x = 0;
      y = (y + wsize / 8) % 8;
    }
    if (y > (8 - wsize / 8)) {
      y = 0;
    }

    OLED_ShowChar(x, y, *p, wsize);
    x += wsize / 2;
    p++;
  }
}

static void OLED_ShowInt32Num(uint16_t x, uint16_t y, int32_t num, uint8_t len,
                       uint8_t wsize) {
  char show_len[8] = {0}, show_num[12] = {0};
  uint8_t t = 0;

  if (len > 32)
    return;

  sprintf(show_len, "%%%dd", (int)len);
  sprintf(show_num, show_len, (int)num);

  while (*(show_num + t) != 0) {
    OLED_ShowChar(x + (wsize / 2) * t, y, show_num[t], wsize);
    t++;
  }
}
											 

static void OLED_DrawFont16(uint16_t x, uint16_t y, char *s) {
  uint8_t x0 = 0, y0 = 0;
  uint16_t k = 0;
  uint16_t HZnum = 0;

  HZnum = sizeof(tfont16) / sizeof(typFNT_GB16);

  for (k = 0; k < HZnum; k++) {
    if ((tfont16[k].Index[0] == *(s)) && (tfont16[k].Index[1] == *(s + 1))) {
      for (y0 = 0; y0 < 2; y0++) {
        OLED_Set_Pos(x, y + y0);
        for (x0 = 0; x0 < 16; x0++) {
          OLED_WR_Byte(tfont16[k].Msk[y0 * 16 + x0], IIC_OLED_DATA);
        }
      }
      break;
    }
  }
}

static void OLED_DrawFont32(uint16_t x, uint16_t y, char *s) {
  uint8_t x0 = 0, y0 = 0;
  uint16_t k = 0;
  uint16_t HZnum = 0;

  HZnum = sizeof(tfont32) / sizeof(typFNT_GB32);

  for (k = 0; k < HZnum; k++) {
    if ((tfont16[k].Index[0] == *(s)) && (tfont16[k].Index[1] == *(s + 1))) {
      for (y0 = 0; y0 < 4; y0++) {
        OLED_Set_Pos(x, y + y0);
        for (x0 = 0; x0 < 32; x0++) {
          OLED_WR_Byte(tfont32[k].Msk[y0 * 32 + x0], IIC_OLED_DATA);
        }
      }
      break;
    }
  }
}

static void OLED_Show_Str(uint16_t x, uint16_t y, char *str, uint8_t wsize) {
  uint16_t x0 = x;
  uint8_t bHz = 0;

  if (wsize != 32)
    wsize = 16;
  while (*str != 0) {
    if (!bHz) {
      if (x > (128 - wsize / 2)) {
        x = 0;
        y = (y + wsize / 8) % 8;
      }
      if (y > (8 - wsize / 8)) {
        y = 0;
      }
      if ((uint8_t)*str > 0x80) {
        bHz = 1;
      } else {
        if (*str == 0x0D) {
          y += wsize;
          x = x0;
          str++;
        } else {
          OLED_ShowChar(x, y, *str, wsize);
          x += wsize / 2;
        }
        str++;
      }
    } else {
      if (x > (128 - wsize)) {
        x = 0;
        y = (y + wsize / 8) % 8;
      }
      if (y > (8 - wsize / 8)) {
        y = 0;
      }
      bHz = 0;
      if (wsize == 32)
        OLED_DrawFont32(x, y, str);
      else if (wsize == 16)
        OLED_DrawFont16(x, y, str);

      str += 2;
      x += wsize;
    }
  }
}

static void OLED_Print(uint16_t x, uint16_t y, uint8_t wsize, char *format, ...) {
	va_list args;
	va_start(args, format);
	int size = vsnprintf(NULL, 0, format, args) + 1; 
	va_end(args);
	if (size <= 0) {
			return; 
	}
	uint8_t *buffer = (uint8_t *)malloc(size);
	if (!buffer) {
			return; 
	}
	va_start(args, format);
	vsnprintf((char *)buffer, size, format, args);
	va_end(args);
	OLED_Show_Str(x, y, (char *)buffer, wsize);
	free(buffer);
}



oled_i oled = {.init = OLED_Init,
               .clear = OLED_Clear,
               .show_char = OLED_ShowChar,
               .show_string = OLED_ShowString,
               .show_int32_num = OLED_ShowInt32Num,
               .draw_font16 = OLED_DrawFont16,
               .draw_font32 = OLED_DrawFont32,
               .show_str = OLED_Show_Str,
							 .draw_bmp = OLED_DrawBMP,
							 .print = OLED_Print,
};
