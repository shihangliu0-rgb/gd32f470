/*!
    \file    bsp_led.h
    \brief   Board LED status indication (EVAL LED1~3)
*/

#ifndef BSP_LED_H
#define BSP_LED_H

#include "gd32f4xx.h"

void bsp_led_init(void);
void bsp_led_all_off(void);
void bsp_led_set_run(uint8_t on);
void bsp_led_set_adc(uint8_t on);
void bsp_led_set_sleep(uint8_t on);
void bsp_led_heartbeat(void);

#endif /* BSP_LED_H */
