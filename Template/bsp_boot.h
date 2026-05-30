/*!
    \file    bsp_boot.h
    \brief   Bootloader control: IAP flag, app jump, version info
*/

#ifndef BSP_BOOT_H
#define BSP_BOOT_H

#include "gd32f4xx.h"

#define BSP_BOOT_MAGIC           0xB007B007U
#define BSP_BOOT_IAP_REQUEST     0x00000001U
#define BSP_BOOT_CTRL_ADDR       FLASH_USER_DATA_ADDR

#define BSP_BOOT_APP_ADDR        ((uint32_t)0x08008000U)
#define BSP_BOOTLOADER_ADDR      ((uint32_t)0x08000000U)

#define BSP_APP_VERSION          "1.1.0"

void bsp_boot_info_print(void);
uint8_t bsp_boot_iap_mode_active(void);
int8_t bsp_boot_request_iap(void);
void bsp_boot_clear_iap(void);
void bsp_boot_system_reset(void);
uint8_t bsp_boot_app_is_valid(uint32_t app_addr);
void bsp_boot_jump_to_app(uint32_t app_addr);

#endif /* BSP_BOOT_H */
