#ifndef __BSP_OLED_H
#define __BSP_OLED_H

#include <stdint.h>
#include "bsp_rtc.h"

/* 0.91 inch 128x32 SSD1306, I2C0 PB6/PB7 (change here if hardware differs) */
#define OLED_I2C_PERIPH             I2C0
#define OLED_I2C_CLK                RCU_I2C0
#define OLED_I2C_GPIO_CLK           RCU_GPIOB
#define OLED_I2C_GPIO_PORT          GPIOB
#define OLED_I2C_SCL_PIN            GPIO_PIN_6
#define OLED_I2C_SDA_PIN            GPIO_PIN_7
#define OLED_I2C_AF                 GPIO_AF_4
#define OLED_I2C_ADDR               0x3CU

#define OLED_WIDTH                  128U
#define OLED_HEIGHT                 32U

void bsp_oled_init(void);
void bsp_oled_clear(void);
void bsp_oled_draw_string(uint8_t x, uint8_t page, const char *str);
void bsp_oled_show_status(uint32_t power_count, const bsp_rtc_datetime_t *datetime);
void bsp_oled_show_lines(const char *line0, const char *line1, const char *line2);

#endif /* __BSP_OLED_H */
