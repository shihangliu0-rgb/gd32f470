#ifndef __BSP_RTC_H
#define __BSP_RTC_H

#include <stdint.h>

#define BSP_RTC_BKP_MAGIC           0x52544331U

typedef struct {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
} bsp_rtc_datetime_t;

void bsp_rtc_init(void);
void bsp_rtc_on_boot(void);
void bsp_rtc_get_datetime(bsp_rtc_datetime_t *datetime);
int8_t bsp_rtc_set_datetime(const bsp_rtc_datetime_t *datetime);
void bsp_rtc_format(const bsp_rtc_datetime_t *datetime, char *buf, uint32_t buf_len);
int8_t bsp_rtc_parse(const char *text, bsp_rtc_datetime_t *datetime);

/* 运行中每秒写入 Flash，供下次上电读取 */
void bsp_rtc_checkpoint(const bsp_rtc_datetime_t *datetime);
/* 上电时从 Flash 快照，运行期间不再被覆盖 */
int8_t bsp_rtc_get_poweroff_time(bsp_rtc_datetime_t *datetime);

#endif /* __BSP_RTC_H */
