/*!
    \file    app_mode.c
    \brief   GPIO key switches work mode; OLED/LED reflect current mode
*/

#include "app_mode.h"
#include "app_cmd.h"
#include "bsp_dac.h"
#include "bsp_flash.h"
#include "bsp_key.h"
#include "bsp_led.h"
#include "bsp_oled.h"
#include "bsp_pmu.h"
#include "bsp_pt100.h"
#include "bsp_uart_dma.h"
#include "systick.h"
#include <stdio.h>

static app_mode_t current_mode = APP_MODE_STATUS;
static uint16_t dac_out0_mv = 1650U;
static uint16_t dac_out1_mv = 1650U;
static uint32_t last_ui_refresh_ms = 0U;

static const char *mode_names[] = {
    "STATUS",
    "ADC",
    "DAC",
    "SLEEP"
};

const char *app_mode_name(app_mode_t mode)
{
    if(mode >= APP_MODE_COUNT) {
        return "UNKNOWN";
    }

    return mode_names[mode];
}

app_mode_t app_mode_get(void)
{
    return current_mode;
}

static void app_mode_apply_led(void)
{
    bsp_led_all_off();

    switch(current_mode) {
    case APP_MODE_STATUS:
        bsp_led_set_run(1U);
        break;
    case APP_MODE_ADC:
        bsp_led_set_adc(1U);
        break;
    case APP_MODE_DAC:
        bsp_led_set_run(1U);
        bsp_led_set_adc(1U);
        break;
    case APP_MODE_SLEEP:
        bsp_led_set_sleep(1U);
        break;
    default:
        break;
    }
}

static void app_mode_on_enter(app_mode_t mode)
{
    current_mode = mode;
    app_mode_apply_led();

    switch(mode) {
    case APP_MODE_DAC:
        bsp_dac_init();
        bsp_dac_set_voltage_mv(dac_out0_mv, dac_out1_mv);
        break;
    case APP_MODE_SLEEP:
        bsp_pmu_init();
        break;
    default:
        break;
    }

    app_mode_refresh_oled();
    printf("Mode -> %s\r\n", app_mode_name(current_mode));
}

void app_mode_init(void)
{
    bsp_led_init();
    bsp_key_init();
    bsp_pmu_init();
    current_mode = APP_MODE_STATUS;
    app_mode_apply_led();
    last_ui_refresh_ms = systick_ms_get();
}

void app_mode_refresh_oled(void)
{
    bsp_rtc_datetime_t datetime;
    adc_sample_t sample;
    char line0[22];
    char line1[22];
    char line2[22];

    bsp_rtc_get_datetime(&datetime);

    switch(current_mode) {
    case APP_MODE_ADC:
        pt100_measure_sample(&sample);
        snprintf(line0, sizeof(line0), "Mode:ADC");
        snprintf(line1, sizeof(line1), "T=%.1fC", sample.temperature);
        snprintf(line2, sizeof(line2), "R=%.1f", sample.resistance);
        bsp_oled_show_lines(line0, line1, line2);
        break;

    case APP_MODE_DAC:
        snprintf(line0, sizeof(line0), "Mode:DAC");
        snprintf(line1, sizeof(line1), "O0:%umV", (unsigned int)dac_out0_mv);
        snprintf(line2, sizeof(line2), "O1:%umV", (unsigned int)dac_out1_mv);
        bsp_oled_show_lines(line0, line1, line2);
        break;

    case APP_MODE_SLEEP:
        snprintf(line0, sizeof(line0), "Mode:SLEEP");
        snprintf(line1, sizeof(line1), "%02u:%02u:%02u",
                 (unsigned int)datetime.hour,
                 (unsigned int)datetime.minute,
                 (unsigned int)datetime.second);
        snprintf(line2, sizeof(line2), "Key/TAMPER wake");
        bsp_oled_show_lines(line0, line1, line2);
        break;

    default:
        bsp_oled_show_status(read_power_count(), &datetime);
        break;
    }
}

void app_mode_poll(void)
{
    bsp_key_event_t key;
    uint32_t now = systick_ms_get();

    key = bsp_key_poll();
    if(BSP_KEY_USER == key) {
        app_mode_t next = (app_mode_t)(((uint32_t)current_mode + 1U) % (uint32_t)APP_MODE_COUNT);
        app_mode_on_enter(next);
    } else if(BSP_KEY_TAMPER == key) {
        printf("Enter deepsleep, press WAKEUP/TAMPER to wake...\r\n");
        bsp_led_set_sleep(1U);
        bsp_pmu_enter_deepsleep();
        printf("Wakeup from deepsleep.\r\n");
        app_mode_apply_led();
        app_mode_refresh_oled();
    }

    if((now - last_ui_refresh_ms) >= 500U) {
        if(APP_MODE_STATUS != current_mode) {
            app_mode_refresh_oled();
        }
        if(APP_MODE_STATUS == current_mode) {
            bsp_led_heartbeat();
        }
        last_ui_refresh_ms = now;
    }

    if((APP_MODE_SLEEP == current_mode) && (0U == uart_cmd_len)) {
        bsp_pmu_enter_sleep();
    }
}

void app_mode_set_dac_mv(uint16_t out0_mv, uint16_t out1_mv)
{
    dac_out0_mv = out0_mv;
    dac_out1_mv = out1_mv;

    if(APP_MODE_DAC == current_mode) {
        bsp_dac_set_voltage_mv(dac_out0_mv, dac_out1_mv);
        app_mode_refresh_oled();
    }
}

uint16_t app_mode_get_dac0_mv(void)
{
    return dac_out0_mv;
}

uint16_t app_mode_get_dac1_mv(void)
{
    return dac_out1_mv;
}
