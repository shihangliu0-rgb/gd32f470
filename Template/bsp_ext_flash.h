/*!
    \file    bsp_ext_flash.h
    \brief   External SPI NOR flash (GD25Qxx on SPI5, eval-board pins)
*/

#ifndef BSP_EXT_FLASH_H
#define BSP_EXT_FLASH_H

#include "gd32f4xx.h"

#define EXT_FLASH_PAGE_SIZE    256U
#define EXT_FLASH_SECTOR_SIZE  4096U

int8_t bsp_ext_flash_init(void);
uint32_t bsp_ext_flash_read_id(void);
int8_t bsp_ext_flash_read(uint32_t addr, uint8_t *buf, uint32_t len);
int8_t bsp_ext_flash_write(uint32_t addr, const uint8_t *buf, uint32_t len);
int8_t bsp_ext_flash_sector_erase(uint32_t addr);
uint8_t bsp_ext_flash_is_ready(void);

#endif /* BSP_EXT_FLASH_H */
