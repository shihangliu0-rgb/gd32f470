#include "bsp_rtc.h"
#include "bsp_flash.h"
#include "gd32f4xx.h"
#include "gd32f4xx_rtc.h"
#include "gd32f4xx_pmu.h"
#include <stdio.h>
#include <string.h>

#define RTC_CLOCK_SOURCE_LXTAL
#define RTC_BKP_MAGIC_VALUE         BSP_RTC_BKP_MAGIC

static uint32_t rtc_prescaler_a = 0U;
static uint32_t rtc_prescaler_s = 0U;
static bsp_rtc_datetime_t g_poweroff_time;
static uint8_t g_poweroff_time_valid = 0U;

static uint8_t dec_to_bcd(uint8_t value);
static uint8_t bcd_to_dec(uint8_t value);
static uint8_t month_to_rtc(uint8_t month);
static uint8_t rtc_to_month(uint8_t month);
static uint8_t calc_weekday(uint16_t year, uint8_t month, uint8_t day);
static void rtc_pre_config(void);
static ErrStatus rtc_apply_datetime(const bsp_rtc_datetime_t *datetime);

static uint8_t dec_to_bcd(uint8_t value)
{
    return (uint8_t)(((value / 10U) << 4) | (value % 10U));
}

static uint8_t bcd_to_dec(uint8_t value)
{
    return (uint8_t)(((value >> 4) * 10U) + (value & 0x0FU));
}

static uint8_t month_to_rtc(uint8_t month)
{
    return month;
}

static uint8_t rtc_to_month(uint8_t month)
{
    return month;
}

static uint8_t calc_weekday(uint16_t year, uint8_t month, uint8_t day)
{
    /* Zeller, return 1=Mon ... 7=Sun for GD32 RTC */
    int y = (int)year;
    int m = (int)month;
    int d = (int)day;
    int h;

    if(m < 3) {
        m += 12;
        y -= 1;
    }

    h = (d + (13 * (m + 1)) / 5 + y + y / 4 - y / 100 + y / 400) % 7;
    /* h: 0=Sat,1=Sun,... convert to Mon=1 */
    return (uint8_t)(((h + 5) % 7) + 1);
}

static void rtc_pre_config(void)
{
#if defined(RTC_CLOCK_SOURCE_LXTAL)
    rcu_osci_on(RCU_LXTAL);
    if(SUCCESS == rcu_osci_stab_wait(RCU_LXTAL)) {
        rcu_rtc_clock_config(RCU_RTCSRC_LXTAL);
        rtc_prescaler_s = 0xFFU;
        rtc_prescaler_a = 0x7FU;
    } else {
        rcu_osci_on(RCU_IRC32K);
        rcu_osci_stab_wait(RCU_IRC32K);
        rcu_rtc_clock_config(RCU_RTCSRC_IRC32K);
        rtc_prescaler_s = 0x13FU;
        rtc_prescaler_a = 0x63U;
    }
#else
    rcu_osci_on(RCU_IRC32K);
    rcu_osci_stab_wait(RCU_IRC32K);
    rcu_rtc_clock_config(RCU_RTCSRC_IRC32K);
    rtc_prescaler_s = 0x13FU;
    rtc_prescaler_a = 0x63U;
#endif

    rcu_periph_clock_enable(RCU_RTC);
    rtc_register_sync_wait();
}

static ErrStatus rtc_apply_datetime(const bsp_rtc_datetime_t *datetime)
{
    rtc_parameter_struct rtc_initpara;

    memset(&rtc_initpara, 0, sizeof(rtc_initpara));
    rtc_initpara.factor_asyn = (uint16_t)rtc_prescaler_a;
    rtc_initpara.factor_syn = (uint16_t)rtc_prescaler_s;
    rtc_initpara.year = dec_to_bcd((uint8_t)(datetime->year % 100U));
    rtc_initpara.month = month_to_rtc(datetime->month);
    rtc_initpara.date = dec_to_bcd(datetime->day);
    rtc_initpara.day_of_week = calc_weekday(datetime->year, datetime->month, datetime->day);
    rtc_initpara.hour = dec_to_bcd(datetime->hour);
    rtc_initpara.minute = dec_to_bcd(datetime->minute);
    rtc_initpara.second = dec_to_bcd(datetime->second);
    rtc_initpara.display_format = RTC_24HOUR;
    rtc_initpara.am_pm = RTC_AM;

    return rtc_init(&rtc_initpara);
}

void bsp_rtc_init(void)
{
    bsp_rtc_datetime_t datetime;
    uint32_t rtc_source;

    rcu_periph_clock_enable(RCU_PMU);
    pmu_backup_write_enable();
    rtc_pre_config();

    rtc_source = GET_BITS(RCU_BDCTL, 8, 9);

    if((RTC_BKP_MAGIC_VALUE != RTC_BKP0) || (0U == rtc_source)) {
        if(0 == bsp_rtc_get_poweroff_time(&datetime)) {
            if(SUCCESS == rtc_apply_datetime(&datetime)) {
                RTC_BKP0 = RTC_BKP_MAGIC_VALUE;
                return;
            }
        }

        datetime.year = 2026U;
        datetime.month = 1U;
        datetime.day = 1U;
        datetime.hour = 0U;
        datetime.minute = 0U;
        datetime.second = 0U;
        if(SUCCESS == rtc_apply_datetime(&datetime)) {
            RTC_BKP0 = RTC_BKP_MAGIC_VALUE;
        }
    } else {
        rtc_register_sync_wait();
    }
}

void bsp_rtc_get_datetime(bsp_rtc_datetime_t *datetime)
{
    rtc_parameter_struct rtc_time;

    if(NULL == datetime) {
        return;
    }

    rtc_current_time_get(&rtc_time);
    datetime->year = (uint16_t)(2000U + bcd_to_dec(rtc_time.year));
    datetime->month = rtc_to_month(rtc_time.month);
    datetime->day = bcd_to_dec(rtc_time.date);
    datetime->hour = bcd_to_dec(rtc_time.hour);
    datetime->minute = bcd_to_dec(rtc_time.minute);
    datetime->second = bcd_to_dec(rtc_time.second);
}

int8_t bsp_rtc_set_datetime(const bsp_rtc_datetime_t *datetime)
{
    if(NULL == datetime) {
        return -1;
    }

    if((datetime->month < 1U) || (datetime->month > 12U) ||
       (datetime->day < 1U) || (datetime->day > 31U) ||
       (datetime->hour > 23U) || (datetime->minute > 59U) || (datetime->second > 59U)) {
        return -2;
    }

    if(SUCCESS != rtc_apply_datetime(datetime)) {
        return -3;
    }

    RTC_BKP0 = RTC_BKP_MAGIC_VALUE;
    return 0;
}

void bsp_rtc_on_boot(void)
{
    flash_user_data_t data;

    g_poweroff_time_valid = 0U;
    flash_read(FLASH_USER_DATA_ADDR, (uint32_t *)&data, (sizeof(flash_user_data_t) + 3U) / 4U);

    if((FLASH_USER_DATA_MAGIC == data.magic) && (RTC_LAST_TIME_MAGIC == data.last_time_magic)) {
        g_poweroff_time.year = data.last_year;
        g_poweroff_time.month = data.last_month;
        g_poweroff_time.day = data.last_day;
        g_poweroff_time.hour = data.last_hour;
        g_poweroff_time.minute = data.last_minute;
        g_poweroff_time.second = data.last_second;
        g_poweroff_time_valid = 1U;
    }
}

int8_t bsp_rtc_get_poweroff_time(bsp_rtc_datetime_t *datetime)
{
    if((NULL == datetime) || (0U == g_poweroff_time_valid)) {
        return -1;
    }

    *datetime = g_poweroff_time;
    return 0;
}

void bsp_rtc_format(const bsp_rtc_datetime_t *datetime, char *buf, uint32_t buf_len)
{
    if((NULL == datetime) || (NULL == buf) || (buf_len < 20U)) {
        return;
    }

    snprintf(buf, buf_len, "%04u-%02u-%02u %02u:%02u:%02u",
             (unsigned int)datetime->year,
             (unsigned int)datetime->month,
             (unsigned int)datetime->day,
             (unsigned int)datetime->hour,
             (unsigned int)datetime->minute,
             (unsigned int)datetime->second);
}
//数据分析
int8_t bsp_rtc_parse(const char *text, bsp_rtc_datetime_t *datetime)
{
    unsigned int year;
    unsigned int month;
    unsigned int day;
    unsigned int hour;
    unsigned int minute;
    unsigned int second;

    if((NULL == text) || (NULL == datetime)) {
        return -1;
    }

    if(6 != sscanf(text, "%u-%u-%u %u:%u:%u",
                   &year, &month, &day, &hour, &minute, &second)) {
        return -2;
    }

    if((year < 2000U) || (year > 2099U) ||
       (month < 1U) || (month > 12U) ||
       (day < 1U) || (day > 31U) ||
       (hour > 23U) || (minute > 59U) || (second > 59U)) {
        return -3;
    }

    datetime->year = (uint16_t)year;
    datetime->month = (uint8_t)month;
    datetime->day = (uint8_t)day;
    datetime->hour = (uint8_t)hour;
    datetime->minute = (uint8_t)minute;
    datetime->second = (uint8_t)second;
    return 0;
}

void bsp_rtc_checkpoint(const bsp_rtc_datetime_t *datetime)
{
    flash_user_data_t data;

    if(NULL == datetime) {
        return;
    }

    flash_user_data_load(&data);
    data.magic = FLASH_USER_DATA_MAGIC;
    data.last_time_magic = RTC_LAST_TIME_MAGIC;
    data.last_year = datetime->year;
    data.last_month = datetime->month;
    data.last_day = datetime->day;
    data.last_hour = datetime->hour;
    data.last_minute = datetime->minute;
    data.last_second = datetime->second;
    flash_user_data_save(&data);
}
