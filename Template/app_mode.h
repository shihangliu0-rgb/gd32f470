/*!
    \file    app_mode.h
    \brief   System work mode: key switch + OLED/LED/DAC/PMU integration
*/

#ifndef APP_MODE_H
#define APP_MODE_H

#include "bsp_rtc.h"
#include <stdint.h>

typedef enum {
    APP_MODE_STATUS = 0,
    APP_MODE_ADC,
    APP_MODE_DAC,
    APP_MODE_SLEEP,
    APP_MODE_COUNT
} app_mode_t;

void app_mode_init(void);
void app_mode_poll(void);
app_mode_t app_mode_get(void);
const char *app_mode_name(app_mode_t mode);
void app_mode_refresh_oled(void);
void app_mode_set_dac_mv(uint16_t out0_mv, uint16_t out1_mv);
uint16_t app_mode_get_dac0_mv(void);
uint16_t app_mode_get_dac1_mv(void);

#endif /* APP_MODE_H */
