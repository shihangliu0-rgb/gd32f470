/*!
    \file    bsp_boot.c
    \brief   Boot/IAP flag in internal flash user-data sector
*/

#include "bsp_boot.h"
#include "bsp_flash.h"
#include <stdio.h>

static int8_t bsp_boot_update_flag(uint32_t flag)
{
    flash_user_data_t data;

    flash_user_data_load(&data);
    data.boot_magic = BSP_BOOT_MAGIC;
    data.boot_flag = flag;
    return flash_user_data_save(&data);
}

void bsp_boot_info_print(void)
{
    flash_user_data_t data;

    flash_user_data_load(&data);
    printf("App version: %s\r\n", BSP_APP_VERSION);
    printf("Boot ctrl: magic=0x%08lX flag=0x%08lX\r\n",
           (unsigned long)data.boot_magic, (unsigned long)data.boot_flag);
    printf("App addr: 0x%08lX, Boot addr: 0x%08lX\r\n",
           (unsigned long)BSP_BOOT_APP_ADDR, (unsigned long)BSP_BOOTLOADER_ADDR);
}

uint8_t bsp_boot_iap_mode_active(void)
{
    flash_user_data_t data;

    flash_user_data_load(&data);
    if((BSP_BOOT_MAGIC == data.boot_magic) &&
       (0U != (data.boot_flag & BSP_BOOT_IAP_REQUEST))) {
        return 1U;
    }

    return 0U;
}

int8_t bsp_boot_request_iap(void)
{
    return bsp_boot_update_flag(BSP_BOOT_IAP_REQUEST);
}

void bsp_boot_clear_iap(void)
{
    (void)bsp_boot_update_flag(0U);
}

void bsp_boot_system_reset(void)
{
    NVIC_SystemReset();
}

uint8_t bsp_boot_app_is_valid(uint32_t app_addr)
{
    uint32_t sp = *(volatile uint32_t *)app_addr;
    uint32_t pc = *(volatile uint32_t *)(app_addr + 4U);

    if((sp & 0x2FF00000U) != 0x20000000U) {
        return 0U;
    }

    if((pc < 0x08000000U) || (pc >= FLASH_END_ADDR)) {
        return 0U;
    }

    return 1U;
}

void bsp_boot_jump_to_app(uint32_t app_addr)
{
    typedef void (*app_entry_t)(void);
    uint32_t sp = *(volatile uint32_t *)app_addr;
    uint32_t pc = *(volatile uint32_t *)(app_addr + 4U);
    app_entry_t entry;
    uint32_t i;

    if(0U == bsp_boot_app_is_valid(app_addr)) {
        return;
    }

    __disable_irq();
    SysTick->CTRL = 0U;
    for(i = 0U; i < 8U; i++) {
        NVIC->ICER[i] = 0xFFFFFFFFU;
    }

    SCB->VTOR = app_addr;
    __set_MSP(sp);
    entry = (app_entry_t)pc;
    entry();
}
