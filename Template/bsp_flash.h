#ifndef __BSP_FLASH_H
#define __BSP_FLASH_H

#include "gd32f4xx.h"

/* GD32F470VE: 512KB internal flash, sectors 0~7 (0x08000000~0x0807FFFF).
 * Sectors 8~27 below are for larger-density parts; do not use on 512KB MCU. */

/* --- Bank 0 (sectors 0~11, first 1MB of dual-bank map) --- */
#define ADDR_FLASH_SECTOR_0     ((uint32_t)0x08000000U) /* 16 KB  */
#define ADDR_FLASH_SECTOR_1     ((uint32_t)0x08004000U) /* 16 KB  */
#define ADDR_FLASH_SECTOR_2     ((uint32_t)0x08008000U) /* 16 KB  */
#define ADDR_FLASH_SECTOR_3     ((uint32_t)0x0800C000U) /* 16 KB  */
#define ADDR_FLASH_SECTOR_4     ((uint32_t)0x08010000U) /* 64 KB  */
#define ADDR_FLASH_SECTOR_5     ((uint32_t)0x08020000U) /* 128 KB */
#define ADDR_FLASH_SECTOR_6     ((uint32_t)0x08040000U) /* 128 KB */
#define ADDR_FLASH_SECTOR_7     ((uint32_t)0x08060000U) /* 128 KB */
#define ADDR_FLASH_SECTOR_8     ((uint32_t)0x08080000U) /* 128 KB */
#define ADDR_FLASH_SECTOR_9     ((uint32_t)0x080A0000U) /* 128 KB */
#define ADDR_FLASH_SECTOR_10    ((uint32_t)0x080C0000U) /* 128 KB */
#define ADDR_FLASH_SECTOR_11    ((uint32_t)0x080E0000U) /* 128 KB */

/* --- Bank 1 --- */
#define ADDR_FLASH_SECTOR_12    ((uint32_t)0x08100000U) /* 16 KB  */
#define ADDR_FLASH_SECTOR_13    ((uint32_t)0x08104000U) /* 16 KB  */
#define ADDR_FLASH_SECTOR_14    ((uint32_t)0x08108000U) /* 16 KB  */
#define ADDR_FLASH_SECTOR_15    ((uint32_t)0x0810C000U) /* 16 KB  */
#define ADDR_FLASH_SECTOR_16    ((uint32_t)0x08110000U) /* 64 KB  */
#define ADDR_FLASH_SECTOR_17    ((uint32_t)0x08120000U) /* 128 KB */
#define ADDR_FLASH_SECTOR_18    ((uint32_t)0x08140000U) /* 128 KB */
#define ADDR_FLASH_SECTOR_19    ((uint32_t)0x08160000U) /* 128 KB */
#define ADDR_FLASH_SECTOR_20    ((uint32_t)0x08180000U) /* 128 KB */
#define ADDR_FLASH_SECTOR_21    ((uint32_t)0x081A0000U) /* 128 KB */
#define ADDR_FLASH_SECTOR_22    ((uint32_t)0x081C0000U) /* 128 KB */
#define ADDR_FLASH_SECTOR_23    ((uint32_t)0x081E0000U) /* 128 KB */

/* 256KB large sectors (3MB parts) */
#define ADDR_FLASH_SECTOR_24    ((uint32_t)0x08200000U) /* 256 KB */
#define ADDR_FLASH_SECTOR_25    ((uint32_t)0x08240000U) /* 256 KB */
#define ADDR_FLASH_SECTOR_26    ((uint32_t)0x08280000U) /* 256 KB */
#define ADDR_FLASH_SECTOR_27    ((uint32_t)0x082C0000U) /* 256 KB */

/* 512KB part end address (GD32F470VE) */
#define FLASH_END_ADDR          ((uint32_t)0x08080000U)

/* FMC sector numbers (match gd32f4xx_fmc.h CTL_SECTOR_NUMBER_x) */
#define FMC_SECTOR_0      CTL_SECTOR_NUMBER_0
#define FMC_SECTOR_1      CTL_SECTOR_NUMBER_1
#define FMC_SECTOR_2      CTL_SECTOR_NUMBER_2
#define FMC_SECTOR_3      CTL_SECTOR_NUMBER_3
#define FMC_SECTOR_4      CTL_SECTOR_NUMBER_4
#define FMC_SECTOR_5      CTL_SECTOR_NUMBER_5
#define FMC_SECTOR_6      CTL_SECTOR_NUMBER_6
#define FMC_SECTOR_7      CTL_SECTOR_NUMBER_7
#define FMC_SECTOR_8      CTL_SECTOR_NUMBER_8
#define FMC_SECTOR_9      CTL_SECTOR_NUMBER_9
#define FMC_SECTOR_10     CTL_SECTOR_NUMBER_10
#define FMC_SECTOR_11     CTL_SECTOR_NUMBER_11
#define FMC_SECTOR_24     CTL_SECTOR_NUMBER_24
#define FMC_SECTOR_25     CTL_SECTOR_NUMBER_25
#define FMC_SECTOR_26     CTL_SECTOR_NUMBER_26
#define FMC_SECTOR_27     CTL_SECTOR_NUMBER_27
#define FMC_SECTOR_12     CTL_SECTOR_NUMBER_12
#define FMC_SECTOR_13     CTL_SECTOR_NUMBER_13
#define FMC_SECTOR_14     CTL_SECTOR_NUMBER_14
#define FMC_SECTOR_15     CTL_SECTOR_NUMBER_15
#define FMC_SECTOR_16     CTL_SECTOR_NUMBER_16
#define FMC_SECTOR_17     CTL_SECTOR_NUMBER_17
#define FMC_SECTOR_18     CTL_SECTOR_NUMBER_18
#define FMC_SECTOR_19     CTL_SECTOR_NUMBER_19
#define FMC_SECTOR_20     CTL_SECTOR_NUMBER_20
#define FMC_SECTOR_21     CTL_SECTOR_NUMBER_21
#define FMC_SECTOR_22     CTL_SECTOR_NUMBER_22
#define FMC_SECTOR_23     CTL_SECTOR_NUMBER_23

/* User data: last two sectors of 512KB flash (do not overlap with program) */
#define FLASH_USER_DATA_ADDR    ADDR_FLASH_SECTOR_6
#define POWER_COUNT_ADDR        FLASH_USER_DATA_ADDR
#define ADC_DATA_ADDR           ADDR_FLASH_SECTOR_7

#define FLASH_USER_DATA_MAGIC   0x55534431U
#define RTC_LAST_TIME_MAGIC     0xA55A5AA5U

#define ADC_FLASH_MAGIC         0xADC020B0U
#define ADC_SAMPLE_COUNT        10U

typedef struct {
    uint32_t magic;
    uint32_t power_count;
    uint32_t last_time_magic;
    uint16_t last_year;
    uint8_t last_month;
    uint8_t last_day;
    uint8_t last_hour;
    uint8_t last_minute;
    uint8_t last_second;
    uint8_t reserved;
    uint32_t boot_magic;
    uint32_t boot_flag;
} flash_user_data_t;

typedef struct {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    int16_t raw;
    float voltage;
    float resistance;
    float temperature;
} adc_sample_t;

typedef struct {
    uint32_t magic;
    uint32_t count;
    adc_sample_t samples[ADC_SAMPLE_COUNT];
} adc_flash_record_t;

int8_t flash_erase_address(uint32_t flash_address);
int8_t flash_write_single_address(uint32_t start_address, uint32_t *buf, uint32_t len);
int8_t flash_write_muli_address(uint32_t start_address, uint32_t end_address, uint32_t *buf, uint32_t len);
void flash_read(uint32_t address, uint32_t *buf, uint32_t len);
uint32_t get_sector(uint32_t address);
uint32_t get_next_flash_address(uint32_t address);

int8_t flash_user_data_load(flash_user_data_t *data);
int8_t flash_user_data_save(const flash_user_data_t *data);

uint32_t read_power_count(void);
int8_t save_power_count(void);
int8_t save_adc_record(const adc_flash_record_t *record);
int8_t load_adc_record(adc_flash_record_t *record);

#endif /* __BSP_FLASH_H */
