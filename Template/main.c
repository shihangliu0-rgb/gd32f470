/*!
    \file    main.c
    \brief   PT100 temperature sampling board application entry
*/

#include "gd32f4xx.h"
#include "gd32f450i_eval.h"
#include "systick.h"
#include "bsp_flash.h"
#include "bsp_rtc.h"
#include "bsp_oled.h"
#include "bsp_pt100.h"
#include "bsp_uart_dma.h"
#include "bsp_boot.h"
#include "app_cmd.h"
#include "app_mode.h"
#include <stdio.h>

#define OLED_REFRESH_MS    500U
#define RTC_BACKUP_MS      1000U

static uint32_t last_oled_refresh_ms = 0U;
static uint32_t last_rtc_backup_ms = 0U;

static void app_periodic_task(void);

int main(void)
{
    int8_t power_ret;
    bsp_rtc_datetime_t datetime;

    systick_config();
    gd_eval_com_init(EVAL_COM0);
    bsp_uart_dma_init();

    delay_1ms(50);
    printf("\r\nPT100 board starting...\r\n");

    pt100_init();
    bsp_rtc_on_boot();
    bsp_rtc_init();
    bsp_oled_init();
    app_mode_init();

    delay_1ms(10);
    printf("PT100 board init done.\r\n");

    if(0U != bsp_boot_iap_mode_active()) {
        printf("IAP mode active. Use BOOT CLEAR to exit.\r\n");
    }

    power_ret = save_power_count();
    if(0 == power_ret) {
        printf("Power-on count: %lu\r\n", (unsigned long)read_power_count());
    } else {
        printf("Power-on count save failed: %d\r\n", power_ret);
    }

    if(0 == bsp_rtc_get_poweroff_time(&datetime)) {
        char time_text[24];

        bsp_rtc_format(&datetime, time_text, sizeof(time_text));
        printf("Last power-off time: %s\r\n", time_text);
    } else {
        printf("Last power-off time: not available\r\n");
    }

    app_mode_refresh_oled();

    printf("Commands: START adc | ADC | RTC | DAC | SLEEP | FLASH | BOOT\r\n");
    printf("Keys: USER=switch mode, TAMPER=deepsleep\r\n");
    uart_print_prompt();

    last_oled_refresh_ms = systick_ms_get();
    last_rtc_backup_ms = last_oled_refresh_ms;

    while(1) {
        bsp_uart_dma_poll();
        app_mode_poll();
        app_periodic_task();
    }
}

static void app_periodic_task(void)
{
    uint32_t now = systick_ms_get();
    bsp_rtc_datetime_t datetime;

    if((APP_MODE_STATUS == app_mode_get()) &&
       ((now - last_oled_refresh_ms) >= OLED_REFRESH_MS)) {
        bsp_rtc_get_datetime(&datetime);
        bsp_oled_show_status(read_power_count(), &datetime);
        last_oled_refresh_ms = now;
    }

    if((now - last_rtc_backup_ms) >= RTC_BACKUP_MS) {
        if(0U == uart_cmd_len) {
            bsp_rtc_get_datetime(&datetime);
            bsp_rtc_checkpoint(&datetime);
        }
        last_rtc_backup_ms = now;
    }
}
