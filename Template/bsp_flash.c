#include "bsp_flash.h"
#include "gd32f4xx_fmc.h"
#include <stdint.h>
#include <string.h>

int8_t flash_erase_address(uint32_t flash_address)
{
    fmc_state_enum flash_status;

    fmc_unlock();
    fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_OPERR | FMC_FLAG_WPERR | FMC_FLAG_PGMERR | FMC_FLAG_PGSERR);
    flash_status = fmc_sector_erase(get_sector(flash_address));
    fmc_lock();

    return (FMC_READY == flash_status) ? 0 : -1;
}

int8_t flash_write_single_address(uint32_t start_address, uint32_t *buf, uint32_t len)
{
    fmc_state_enum flash_status;
    uint32_t uw_address = start_address;
    uint32_t end_address = get_next_flash_address(start_address);
    uint32_t *data_buf = buf;
    uint32_t data_len = 0U;

    fmc_unlock();
    fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_OPERR | FMC_FLAG_WPERR | FMC_FLAG_PGMERR | FMC_FLAG_PGSERR);

    while(uw_address < end_address) {
        flash_status = fmc_word_program(uw_address, *data_buf);
        if(FMC_READY == flash_status) {
            uw_address += 4U;
            data_buf++;
            data_len++;
            if(data_len == len) {
                break;
            }
        } else {
            fmc_lock();
            return -1;
        }
    }

    fmc_lock();
    return 0;
}

int8_t flash_write_muli_address(uint32_t start_address, uint32_t end_address, uint32_t *buf, uint32_t len)
{
    fmc_state_enum flash_status;
    uint32_t uw_address = start_address;
    uint32_t *data_buf = buf;
    uint32_t data_len = 0U;

    fmc_unlock();
    fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_OPERR | FMC_FLAG_WPERR | FMC_FLAG_PGMERR | FMC_FLAG_PGSERR);

    while(uw_address <= end_address) {
        flash_status = fmc_word_program(uw_address, *data_buf);
        if(FMC_READY == flash_status) {
            uw_address += 4U;
            data_buf++;
            data_len++;
            if(data_len == len) {
                break;
            }
        } else {
            fmc_lock();
            return -1;
        }
    }

    fmc_lock();
    return 0;
}

void flash_read(uint32_t address, uint32_t *buf, uint32_t len)
{
    memcpy(buf, (void *)address, len * 4U);
}

uint32_t get_sector(uint32_t address)
{
    if(address < ADDR_FLASH_SECTOR_1) {
        return FMC_SECTOR_0;
    }
    if(address < ADDR_FLASH_SECTOR_2) {
        return FMC_SECTOR_1;
    }
    if(address < ADDR_FLASH_SECTOR_3) {
        return FMC_SECTOR_2;
    }
    if(address < ADDR_FLASH_SECTOR_4) {
        return FMC_SECTOR_3;
    }
    if(address < ADDR_FLASH_SECTOR_5) {
        return FMC_SECTOR_4;
    }
    if(address < ADDR_FLASH_SECTOR_6) {
        return FMC_SECTOR_5;
    }
    if(address < ADDR_FLASH_SECTOR_7) {
        return FMC_SECTOR_6;
    }
    if(address < ADDR_FLASH_SECTOR_8) {
        return FMC_SECTOR_7;
    }
    if(address < ADDR_FLASH_SECTOR_9) {
        return FMC_SECTOR_8;
    }
    if(address < ADDR_FLASH_SECTOR_10) {
        return FMC_SECTOR_9;
    }
    if(address < ADDR_FLASH_SECTOR_11) {
        return FMC_SECTOR_10;
    }
    if(address < ADDR_FLASH_SECTOR_12) {
        return FMC_SECTOR_11;
    }
    if(address < ADDR_FLASH_SECTOR_13) {
        return FMC_SECTOR_12;
    }
    if(address < ADDR_FLASH_SECTOR_14) {
        return FMC_SECTOR_13;
    }
    if(address < ADDR_FLASH_SECTOR_15) {
        return FMC_SECTOR_14;
    }
    if(address < ADDR_FLASH_SECTOR_16) {
        return FMC_SECTOR_15;
    }
    if(address < ADDR_FLASH_SECTOR_17) {
        return FMC_SECTOR_16;
    }
    if(address < ADDR_FLASH_SECTOR_18) {
        return FMC_SECTOR_17;
    }
    if(address < ADDR_FLASH_SECTOR_19) {
        return FMC_SECTOR_18;
    }
    if(address < ADDR_FLASH_SECTOR_20) {
        return FMC_SECTOR_19;
    }
    if(address < ADDR_FLASH_SECTOR_21) {
        return FMC_SECTOR_20;
    }
    if(address < ADDR_FLASH_SECTOR_22) {
        return FMC_SECTOR_21;
    }
    if(address < ADDR_FLASH_SECTOR_23) {
        return FMC_SECTOR_22;
    }
    if(address < ADDR_FLASH_SECTOR_24) {
        return FMC_SECTOR_23;
    }
    if(address < ADDR_FLASH_SECTOR_25) {
        return FMC_SECTOR_24;
    }
    if(address < ADDR_FLASH_SECTOR_26) {
        return FMC_SECTOR_25;
    }
    if(address < ADDR_FLASH_SECTOR_27) {
        return FMC_SECTOR_26;
    }

    return FMC_SECTOR_27;
}

uint32_t get_next_flash_address(uint32_t address)
{
    if(address < ADDR_FLASH_SECTOR_1) {
        return ADDR_FLASH_SECTOR_1;
    }
    if(address < ADDR_FLASH_SECTOR_2) {
        return ADDR_FLASH_SECTOR_2;
    }
    if(address < ADDR_FLASH_SECTOR_3) {
        return ADDR_FLASH_SECTOR_3;
    }
    if(address < ADDR_FLASH_SECTOR_4) {
        return ADDR_FLASH_SECTOR_4;
    }
    if(address < ADDR_FLASH_SECTOR_5) {
        return ADDR_FLASH_SECTOR_5;
    }
    if(address < ADDR_FLASH_SECTOR_6) {
        return ADDR_FLASH_SECTOR_6;
    }
    if(address < ADDR_FLASH_SECTOR_7) {
        return ADDR_FLASH_SECTOR_7;
    }
    if(address < ADDR_FLASH_SECTOR_8) {
        return ADDR_FLASH_SECTOR_8;
    }
    if(address < ADDR_FLASH_SECTOR_9) {
        return ADDR_FLASH_SECTOR_9;
    }
    if(address < ADDR_FLASH_SECTOR_10) {
        return ADDR_FLASH_SECTOR_10;
    }
    if(address < ADDR_FLASH_SECTOR_11) {
        return ADDR_FLASH_SECTOR_11;
    }
    if(address < ADDR_FLASH_SECTOR_12) {
        return ADDR_FLASH_SECTOR_12;
    }
    if(address < ADDR_FLASH_SECTOR_13) {
        return ADDR_FLASH_SECTOR_13;
    }
    if(address < ADDR_FLASH_SECTOR_14) {
        return ADDR_FLASH_SECTOR_14;
    }
    if(address < ADDR_FLASH_SECTOR_15) {
        return ADDR_FLASH_SECTOR_15;
    }
    if(address < ADDR_FLASH_SECTOR_16) {
        return ADDR_FLASH_SECTOR_16;
    }
    if(address < ADDR_FLASH_SECTOR_17) {
        return ADDR_FLASH_SECTOR_17;
    }
    if(address < ADDR_FLASH_SECTOR_18) {
        return ADDR_FLASH_SECTOR_18;
    }
    if(address < ADDR_FLASH_SECTOR_19) {
        return ADDR_FLASH_SECTOR_19;
    }
    if(address < ADDR_FLASH_SECTOR_20) {
        return ADDR_FLASH_SECTOR_20;
    }
    if(address < ADDR_FLASH_SECTOR_21) {
        return ADDR_FLASH_SECTOR_21;
    }
    if(address < ADDR_FLASH_SECTOR_22) {
        return ADDR_FLASH_SECTOR_22;
    }
    if(address < ADDR_FLASH_SECTOR_23) {
        return ADDR_FLASH_SECTOR_23;
    }
    if(address < ADDR_FLASH_SECTOR_24) {
        return ADDR_FLASH_SECTOR_24;
    }
    if(address < ADDR_FLASH_SECTOR_25) {
        return ADDR_FLASH_SECTOR_25;
    }
    if(address < ADDR_FLASH_SECTOR_26) {
        return ADDR_FLASH_SECTOR_26;
    }
    if(address < ADDR_FLASH_SECTOR_27) {
        return ADDR_FLASH_SECTOR_27;
    }

    return FLASH_END_ADDR;
}

int8_t flash_user_data_load(flash_user_data_t *data)
{
    uint32_t word_len = (sizeof(flash_user_data_t) + 3U) / 4U;

    if(NULL == data) {
        return -1;
    }

    flash_read(FLASH_USER_DATA_ADDR, (uint32_t *)data, word_len);

    if(FLASH_USER_DATA_MAGIC != data->magic) {
        memset(data, 0, sizeof(flash_user_data_t));
        data->magic = FLASH_USER_DATA_MAGIC;
        return -2;
    }

    return 0;
}

int8_t flash_user_data_save(const flash_user_data_t *data)
{
    flash_user_data_t write_data;
    uint32_t word_len = (sizeof(flash_user_data_t) + 3U) / 4U;

    if(NULL == data) {
        return -1;
    }

    write_data = *data;
    write_data.magic = FLASH_USER_DATA_MAGIC;

    if(0 != flash_erase_address(FLASH_USER_DATA_ADDR)) {
        return -2;
    }

    if(0 != flash_write_single_address(FLASH_USER_DATA_ADDR,
                                       (uint32_t *)(uintptr_t)&write_data,
                                       word_len)) {
        return -3;
    }

    return 0;
}

uint32_t read_power_count(void)
{
    flash_user_data_t data;

    flash_user_data_load(&data);
    return data.power_count;
}

int8_t save_power_count(void)
{
    flash_user_data_t data;

    flash_user_data_load(&data);
    data.magic = FLASH_USER_DATA_MAGIC;
    data.power_count++;

    return flash_user_data_save(&data);
}

int8_t save_adc_record(const adc_flash_record_t *record)
{
    uint32_t word_len = (sizeof(adc_flash_record_t) + 3U) / 4U;

    if(NULL == record) {
        return -1;
    }

    if(0 != flash_erase_address(ADC_DATA_ADDR)) {
        return -2;
    }

    if(0 != flash_write_single_address(ADC_DATA_ADDR, (uint32_t *)(uintptr_t)record, word_len)) {
        return -3;
    }

    return 0;
}

int8_t load_adc_record(adc_flash_record_t *record)
{
    uint32_t word_len = (sizeof(adc_flash_record_t) + 3U) / 4U;

    if(NULL == record) {
        return -1;
    }

    flash_read(ADC_DATA_ADDR, (uint32_t *)record, word_len);

    if(ADC_FLASH_MAGIC != record->magic) {
        return -2;
    }

    if(record->count > ADC_SAMPLE_COUNT) {
        return -3;
    }

    return 0;
}
