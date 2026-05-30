/*!
    \file    bsp_uart_dma.h
    \brief   USART0 DMA RX (IDLE) / TX (FTF interrupt) for EVAL_COM0
*/

#ifndef BSP_UART_DMA_H
#define BSP_UART_DMA_H

#include "gd32f4xx.h"
#include <stdint.h>

void bsp_uart_dma_init(void);
void bsp_uart_dma_poll(void);
uint8_t bsp_uart_dma_is_inited(void);
uint8_t bsp_uart_dma_tx_busy(void);
int bsp_uart_dma_putchar(int ch);
int bsp_uart_dma_send(const uint8_t *data, uint16_t len);

void bsp_uart_dma_rx_over_flag(void);
void bsp_uart_dma_tx_over_flag(void);

#endif /* BSP_UART_DMA_H */
