/*!
    \file    app_cmd.c
    \brief   UART command handlers (ADC / RTC)
*/

#include "app_cmd.h"
#include "app_mode.h"
#include "bsp_boot.h"
#include "bsp_dac.h"
#include "bsp_ext_flash.h"
#include "bsp_flash.h"
#include "bsp_oled.h"
#include "bsp_pmu.h"
#include "bsp_pt100.h"
#include "bsp_rtc.h"
#include "bsp_uart_dma.h"
#include "systick.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ADC_CAPTURE_INTERVAL_MS    1000U

char uart_cmd_buf[UART_CMD_BUF_SIZE];
uint8_t uart_cmd_len = 0U;

static void delay_1ms_poll_uart(uint32_t ms);
static void uart_cmd_trim(char *cmd);
static int uart_cmd_ieq(const char *a, const char *b);
static int uart_cmd_istart(const char *cmd, const char *prefix);
static void cmd_handle_start_adc(void);
static void cmd_handle_adc_dump(void);
static void cmd_handle_rtc(const char *cmd);
static void cmd_handle_dac(const char *cmd);
static void cmd_handle_sleep(const char *cmd);
static void cmd_handle_flash(const char *cmd);
static void cmd_handle_boot(const char *cmd);

void uart_print_prompt(void)
{
    printf("> ");
}

void uart_process_command(char *cmd)
{
//    uart_cmd_trim(cmd);

    if('\0' == cmd[0]) {
        return;
    }

    printf("\r\n");

    if(uart_cmd_ieq(cmd, "ADC")) {
        cmd_handle_adc_dump();
        return;
    }

    if(uart_cmd_ieq(cmd, "START adc")) {
        cmd_handle_start_adc();
        return;
    }

    if(uart_cmd_ieq(cmd, "RTC") || uart_cmd_istart(cmd, "RTC CONFIG")) {
        cmd_handle_rtc(cmd);
        return;
    }

    if(uart_cmd_ieq(cmd, "DAC") || uart_cmd_istart(cmd, "DAC ")) {
        cmd_handle_dac(cmd);
        return;
    }

    if(uart_cmd_ieq(cmd, "SLEEP") || uart_cmd_istart(cmd, "SLEEP ")) {
        cmd_handle_sleep(cmd);
        return;
    }

    if(uart_cmd_ieq(cmd, "FLASH") || uart_cmd_istart(cmd, "FLASH ")) {
        cmd_handle_flash(cmd);
        return;
    }

    if(uart_cmd_ieq(cmd, "BOOT") || uart_cmd_istart(cmd, "BOOT ")) {
        cmd_handle_boot(cmd);
        return;
    }

    printf("Unknown command: %s\r\n", cmd);
    printf("Use: START adc | ADC | RTC | DAC | SLEEP | FLASH | BOOT\r\n");
}

static void delay_1ms_poll_uart(uint32_t ms)
{
    uint32_t start = systick_ms_get();

    while((systick_ms_get() - start) < ms) {
        bsp_uart_dma_poll();
    }
}

static void uart_cmd_trim(char *cmd)
{
    char *start = cmd;
    char *end;

    while(('\0' != *start) && isspace((unsigned char)*start)) {
        start++;
    }

    if(start != cmd) {
        memmove(cmd, start, strlen(start) + 1U);
    }

    end = cmd + strlen(cmd);
    while((end > cmd) && isspace((unsigned char)*(end - 1))) {
        end--;
    }
    *end = '\0';
}
//忽略大小写
static int uart_cmd_ieq(const char *a, const char *b)
{
    while(('\0' != *a) && ('\0' != *b)) {
        if(toupper((unsigned char)*a) != toupper((unsigned char)*b)) {
            return 0;
        }
        a++;
        b++;
    }

    return ('\0' == *a) && ('\0' == *b);
}
//前缀匹配，用于带参数的命令
static int uart_cmd_istart(const char *cmd, const char *prefix)
{
    while(('\0' != *prefix) && ('\0' != *cmd)) {
        if(toupper((unsigned char)*cmd) != toupper((unsigned char)*prefix)) {
            return 0;
        }
        cmd++;
        prefix++;
    }

    return ('\0' == *prefix);
}

static void cmd_handle_start_adc(void)
{
    adc_flash_record_t record;
    uint32_t i;
    int8_t save_ret;

    memset(&record, 0, sizeof(record));
    record.magic = ADC_FLASH_MAGIC;//文件头，读取的时候，要看这个
    record.count = ADC_SAMPLE_COUNT;//保存了几次

    printf("ADC capture start (%u samples, 1s interval)...\r\n", (unsigned int)ADC_SAMPLE_COUNT);

    for(i = 0U; i < ADC_SAMPLE_COUNT; i++) {
        pt100_measure_sample(&record.samples[i]);//计算
        pt100_print_sample_line(i + 1U, &record.samples[i]);
        if((i + 1U) < ADC_SAMPLE_COUNT) {
            delay_1ms_poll_uart(ADC_CAPTURE_INTERVAL_MS);
        }
    }

    save_ret = save_adc_record(&record);
    if(0 == save_ret) {
        printf("ADC data saved to flash.\r\n");
    } else {
        printf("ADC flash save failed: %d\r\n", save_ret);
    }
}

static void cmd_handle_adc_dump(void)
{
    adc_flash_record_t record;
    uint32_t i;
    int8_t load_ret;

    load_ret = load_adc_record(&record);
    if(0 != load_ret) {
        printf("No valid ADC data in flash (err=%d). Use START adc first.\r\n", load_ret);
        return;
    }

    printf("ADC data from flash (%lu samples):\r\n", (unsigned long)record.count);
    for(i = 0U; i < record.count; i++) {
        pt100_print_sample_line(i + 1U, &record.samples[i]);
    }
}

static void cmd_handle_rtc(const char *cmd)
{
    bsp_rtc_datetime_t now;
    bsp_rtc_datetime_t last;
    char time_text[24];
    const char *config_text;
    int8_t ret;

    if(uart_cmd_ieq(cmd, "RTC")) {
        bsp_rtc_get_datetime(&now);
        bsp_rtc_format(&now, time_text, sizeof(time_text));
        printf("Current RTC: %s\r\n", time_text);

        ret = bsp_rtc_get_poweroff_time(&last);
        if(0 == ret) {
            bsp_rtc_format(&last, time_text, sizeof(time_text));
            printf("Last power-off time: %s\r\n", time_text);
        } else {
            printf("Last power-off time: not available\r\n");
        }
        return;
    }

    if(uart_cmd_istart(cmd, "RTC CONFIG")) {
        config_text = cmd + 10;
			//跳过空格
        while(('\0' != *config_text) && isspace((unsigned char)*config_text)) {
            config_text++;
        }
				//往flash里面写入设置的时间
        ret = bsp_rtc_parse(config_text, &now);
        if(0 != ret) {
            printf("RTC CONFIG format error. Example: RTC CONFIG 2026-05-27 14:30:00\r\n");
            return;
        }
				
        ret = bsp_rtc_set_datetime(&now);
        if(0 == ret) {
            bsp_rtc_format(&now, time_text, sizeof(time_text));
            printf("RTC updated: %s\r\n", time_text);
            bsp_oled_show_status(read_power_count(), &now);
        } else {
            printf("RTC set failed: %d\r\n", ret);
        }
    }
}

static void cmd_handle_dac(const char *cmd)
{
    unsigned int mv0 = app_mode_get_dac0_mv();
    unsigned int mv1 = app_mode_get_dac1_mv();
    int parsed;

    if(uart_cmd_ieq(cmd, "DAC")) {
        printf("DAC OUT0=%u mV, OUT1=%u mV\r\n", mv0, mv1);
        return;
    }

    parsed = sscanf(cmd + 4, "%u %u", &mv0, &mv1);
    if(1 == parsed) {
        mv1 = mv0;
    } else if(parsed <= 0) {
        printf("Usage: DAC [out0_mv] [out1_mv]\r\n");
        return;
    }

    if(mv0 > 3300U) {
        mv0 = 3300U;
    }
    if(mv1 > 3300U) {
        mv1 = 3300U;
    }

    bsp_dac_init();
    app_mode_set_dac_mv((uint16_t)mv0, (uint16_t)mv1);
    printf("DAC set: OUT0=%u mV, OUT1=%u mV\r\n", mv0, mv1);
}

static void cmd_handle_sleep(const char *cmd)
{
    if(uart_cmd_istart(cmd, "SLEEP DEEP")) {
        printf("Enter deepsleep...\r\n");
        bsp_pmu_enter_deepsleep();
        printf("Wakeup from deepsleep.\r\n");
        return;
    }

    if(uart_cmd_ieq(cmd, "SLEEP")) {
        printf("Enter sleep (WFI), any interrupt wakes MCU.\r\n");
        bsp_pmu_enter_sleep();
        printf("Wakeup from sleep.\r\n");
        return;
    }

    printf("Usage: SLEEP | SLEEP DEEP\r\n");
}

static void cmd_handle_flash(const char *cmd)
{
    uint32_t addr;
    uint32_t len;
    uint8_t buf[16];
    uint32_t id;
    int8_t ret;

    if(0U == bsp_ext_flash_is_ready()) {
        if(0 != bsp_ext_flash_init()) {
            printf("External flash not available.\r\n");
            return;
        }
    }

    if(uart_cmd_ieq(cmd, "FLASH")) {
        id = bsp_ext_flash_read_id();
        printf("External flash ID: 0x%06lX\r\n", (unsigned long)(id & 0xFFFFFFU));
        return;
    }

    if(uart_cmd_istart(cmd, "FLASH READ")) {
        if(2 != sscanf(cmd + 10, "%lu %lu", (unsigned long *)&addr, (unsigned long *)&len)) {
            printf("Usage: FLASH READ addr len\r\n");
            return;
        }
        if((0U == len) || (len > sizeof(buf))) {
            len = sizeof(buf);
        }
        ret = bsp_ext_flash_read(addr, buf, len);
        if(0 == ret) {
            uint32_t i;
            printf("FLASH READ @0x%08lX:", (unsigned long)addr);
            for(i = 0U; i < len; i++) {
                printf(" %02X", buf[i]);
            }
            printf("\r\n");
        } else {
            printf("FLASH READ failed: %d\r\n", ret);
        }
        return;
    }

    if(uart_cmd_istart(cmd, "FLASH TEST")) {
        const char pattern[] = "GD32EXT";
        ret = bsp_ext_flash_sector_erase(0U);
        if(0 != ret) {
            printf("FLASH erase failed: %d\r\n", ret);
            return;
        }
        ret = bsp_ext_flash_write(0U, (const uint8_t *)pattern, sizeof(pattern));
        if(0 != ret) {
            printf("FLASH write failed: %d\r\n", ret);
            return;
        }
        memset(buf, 0, sizeof(buf));
        ret = bsp_ext_flash_read(0U, buf, sizeof(pattern));
        if((0 == ret) && (0 == memcmp(buf, pattern, sizeof(pattern)))) {
            printf("External flash R/W test OK.\r\n");
        } else {
            printf("External flash R/W test failed.\r\n");
        }
        return;
    }

    printf("Usage: FLASH | FLASH READ addr len | FLASH TEST\r\n");
}

static void cmd_handle_boot(const char *cmd)
{
    if(uart_cmd_ieq(cmd, "BOOT")) {
        bsp_boot_info_print();
        return;
    }

    if(uart_cmd_ieq(cmd, "BOOT IAP")) {
        if(0 == bsp_boot_request_iap()) {
            printf("IAP flag set, resetting...\r\n");
            bsp_boot_system_reset();
        } else {
            printf("IAP flag set failed.\r\n");
        }
        return;
    }

    if(uart_cmd_ieq(cmd, "BOOT CLEAR")) {
        bsp_boot_clear_iap();
        printf("IAP flag cleared.\r\n");
        return;
    }

    if(uart_cmd_ieq(cmd, "BOOT RESET")) {
        bsp_boot_system_reset();
        return;
    }

    printf("Usage: BOOT | BOOT IAP | BOOT CLEAR | BOOT RESET\r\n");
}
