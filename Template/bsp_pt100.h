/*!
    \file    bsp_pt100.h
    \brief   PT100 + GD30AD3344 on SPI3 (PE11~PE14)

    Layered architecture:
      [驱动层] SPI + DRDY + DMA
      [采样层] 状态机 + 滤波 + 校验
      [应用层] PT100温度 + RTC + 上报
*/

#ifndef BSP_PT100_H
#define BSP_PT100_H

#include "bsp_flash.h"

void pt100_init(void);
void pt100_measure_sample(adc_sample_t *sample);
void pt100_print_sample_line(uint32_t index, const adc_sample_t *sample);
void bsp_pt100_spi_dma_rx_over_flag(void);

#endif /* BSP_PT100_H */
