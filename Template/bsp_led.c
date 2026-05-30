/*!
    \file    bsp_led.c
    \brief   LED1=run, LED2=ADC, LED3=sleep/low-power
*/

#include "bsp_led.h"
#include "gd32f450i_eval.h"
#include "systick.h"

void bsp_led_init(void)
{
    gd_eval_led_init(LED1);
    gd_eval_led_init(LED2);
    gd_eval_led_init(LED3);
    bsp_led_all_off();
}

void bsp_led_all_off(void)
{
    gd_eval_led_off(LED1);
    gd_eval_led_off(LED2);
    gd_eval_led_off(LED3);
}

void bsp_led_set_run(uint8_t on)
{
    if(0U != on) {
        gd_eval_led_on(LED1);
    } else {
        gd_eval_led_off(LED1);
    }
}

void bsp_led_set_adc(uint8_t on)
{
    if(0U != on) {
        gd_eval_led_on(LED2);
    } else {
        gd_eval_led_off(LED2);
    }
}

void bsp_led_set_sleep(uint8_t on)
{
    if(0U != on) {
        gd_eval_led_on(LED3);
    } else {
        gd_eval_led_off(LED3);
    }
}

void bsp_led_heartbeat(void)
{
    static uint32_t last_ms = 0U;

    if((systick_ms_get() - last_ms) >= 500U) {
        gd_eval_led_toggle(LED1);
        last_ms = systick_ms_get();
    }
}
