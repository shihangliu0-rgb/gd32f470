/*!
    \file    bsp_dac.h
    \brief   GD32 on-chip DAC (PA4/PA5)
*/

#ifndef BSP_DAC_H
#define BSP_DAC_H

#include "gd32f4xx.h"

void bsp_dac_init(void);
void bsp_dac_set_voltage_mv(uint16_t out0_mv, uint16_t out1_mv);
void bsp_dac_disable(void);

#endif /* BSP_DAC_H */
