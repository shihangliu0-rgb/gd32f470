/*!
    \file    bsp_uart_dma.c
    \brief   USART0 on EVAL_COM0: DMA1 CH2 RX + IDLE, DMA1 CH7 TX + FTF IRQ
*/

#include "bsp_uart_dma.h"
#include "app_cmd.h"
#include "gd32f450i_eval.h"
#include <string.h>
#include <stdio.h>

#define USART0_DATA_ADDRESS    ((uint32_t)&USART_DATA(USART0))
#define UART_DMA_RX_SIZE       256U
#define UART_DMA_TX_BUF_SIZE   512U

static uint8_t uart_dma_rx_buf[UART_DMA_RX_SIZE];
static uint8_t uart_dma_tx_buf[UART_DMA_TX_BUF_SIZE];
static volatile uint8_t uart_dma_inited = 0U;
static volatile uint8_t uart_dma_tx_busy_flag = 0U;
static volatile uint8_t uart_dma_rx_pending = 0U;
static volatile uint16_t uart_dma_rx_len = 0U;

static void uart_dma_nvic_config(void);
static void uart_dma_periph_config(void);
static void uart_dma_rx_reconfig(void);
static void uart_dma_process_rx_chunk(const uint8_t *data, uint16_t len);

void bsp_uart_dma_init(void)
{
    rcu_periph_clock_enable(RCU_DMA1);

    uart_dma_nvic_config();
    uart_dma_periph_config();
    uart_dma_rx_reconfig();

    usart_interrupt_enable(EVAL_COM0, USART_INT_IDLE);
    uart_dma_inited = 1U;
}

uint8_t bsp_uart_dma_is_inited(void)
{
    return uart_dma_inited;
}

uint8_t bsp_uart_dma_tx_busy(void)
{
    return uart_dma_tx_busy_flag;
}

int bsp_uart_dma_send(const uint8_t *data, uint16_t len)
{
    if((NULL == data) || (0U == len) || (len > UART_DMA_TX_BUF_SIZE)) {
        return -1;
    }

    while(0U != uart_dma_tx_busy_flag) {
    }
		
    memcpy(uart_dma_tx_buf, data, len);
    uart_dma_tx_busy_flag = 1U;
		
    dma_channel_disable(DMA1, DMA_CH7);
    dma_flag_clear(DMA1, DMA_CH7, DMA_FLAG_FTF);
    dma_memory_address_config(DMA1, DMA_CH7, DMA_MEMORY_0, (uint32_t)uart_dma_tx_buf);
    dma_transfer_number_config(DMA1, DMA_CH7, len);
    dma_channel_enable(DMA1, DMA_CH7);

    return 0;
}
//printf
int bsp_uart_dma_putchar(int ch)
{
    uint8_t byte = (uint8_t)ch;

    if(0 != bsp_uart_dma_send(&byte, 1U)) {
        return EOF;
    }

    while(0U != uart_dma_tx_busy_flag) {
    }

    return ch;
}

void bsp_uart_dma_poll(void)
{
	//防止dma数据被修改
    uint8_t local_buf[UART_DMA_RX_SIZE];
    uint16_t local_len;

    if(0U == uart_dma_rx_pending) {
        return;
    }

    local_len = uart_dma_rx_len;
    if(local_len > UART_DMA_RX_SIZE) {
        local_len = UART_DMA_RX_SIZE;
    }
    memcpy(local_buf, uart_dma_rx_buf, local_len);
    uart_dma_rx_pending = 0U;

    uart_dma_process_rx_chunk(local_buf, local_len);
}
//接收完通知
void bsp_uart_dma_rx_over_flag(void)
{
    uint16_t received;
		//标志位没有产生，返回
    if(RESET == usart_interrupt_flag_get(EVAL_COM0, USART_INT_FLAG_IDLE)) {
        return;
    }

    //清除，idle，数据接收一下
    (void)usart_data_receive(EVAL_COM0);
		
		//进行dma的配置
    dma_channel_disable(DMA1, DMA_CH2);
    received = (uint16_t)(UART_DMA_RX_SIZE - dma_transfer_number_get(DMA1, DMA_CH2));//dma倒着计数
    if(received > UART_DMA_RX_SIZE) {
        received = UART_DMA_RX_SIZE;
    }

    uart_dma_rx_len = received;
    uart_dma_rx_pending = 1U;

    uart_dma_rx_reconfig();
}
//发完通知
void bsp_uart_dma_tx_over_flag(void)
{
    if(RESET != dma_interrupt_flag_get(DMA1, DMA_CH7, DMA_INT_FLAG_FTF)) {
        dma_interrupt_flag_clear(DMA1, DMA_CH7, DMA_INT_FLAG_FTF);
        uart_dma_tx_busy_flag = 0U;
    }
}

static void uart_dma_nvic_config(void)
{
    nvic_irq_enable(USART0_IRQn, 0U, 0U);
    nvic_irq_enable(DMA1_Channel7_IRQn, 0U, 1U);
}

static void uart_dma_periph_config(void)
{
    dma_single_data_parameter_struct dma_init_struct;

    usart_dma_receive_config(EVAL_COM0, USART_RECEIVE_DMA_ENABLE);
    usart_dma_transmit_config(EVAL_COM0, USART_TRANSMIT_DMA_ENABLE);

    /* USART0 RX: DMA1 CH2 */
    dma_single_data_para_struct_init(&dma_init_struct);
    dma_deinit(DMA1, DMA_CH2);
    dma_init_struct.direction = DMA_PERIPH_TO_MEMORY;
    dma_init_struct.memory0_addr = (uint32_t)uart_dma_rx_buf;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_8BIT;
    dma_init_struct.number = UART_DMA_RX_SIZE;
    dma_init_struct.periph_addr = USART0_DATA_ADDRESS;
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.priority = DMA_PRIORITY_HIGH;
    dma_single_data_mode_init(DMA1, DMA_CH2, &dma_init_struct);
    dma_circulation_disable(DMA1, DMA_CH2);
    dma_channel_subperipheral_select(DMA1, DMA_CH2, DMA_SUBPERI4);

    /* USART0 TX: DMA1 CH7 */
    dma_deinit(DMA1, DMA_CH7);
    dma_init_struct.direction = DMA_MEMORY_TO_PERIPH;
    dma_init_struct.memory0_addr = (uint32_t)uart_dma_tx_buf;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_8BIT;
    dma_init_struct.number = 1U;
    dma_init_struct.periph_addr = USART0_DATA_ADDRESS;
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.priority = DMA_PRIORITY_HIGH;
    dma_single_data_mode_init(DMA1, DMA_CH7, &dma_init_struct);
    dma_circulation_disable(DMA1, DMA_CH7);
    dma_channel_subperipheral_select(DMA1, DMA_CH7, DMA_SUBPERI4);
    dma_interrupt_enable(DMA1, DMA_CH7, DMA_CHXCTL_FTFIE);
}

//dma，每次弄完之后都需要重新配置
static void uart_dma_rx_reconfig(void)
{
    dma_channel_disable(DMA1, DMA_CH2);
    dma_flag_clear(DMA1, DMA_CH2, DMA_FLAG_FTF);
    dma_memory_address_config(DMA1, DMA_CH2, DMA_MEMORY_0, (uint32_t)uart_dma_rx_buf);
    dma_transfer_number_config(DMA1, DMA_CH2, UART_DMA_RX_SIZE);//设置dma接收多少数据
    dma_channel_enable(DMA1, DMA_CH2);
}

static void uart_dma_process_rx_chunk(const uint8_t *data, uint16_t len)
{
    uint16_t i;
    const char *prompt = "> ";
    const char *err = "\r\n!cmd too long\r\n";

    for(i = 0U; i < len; i++) {
        char ch = (char)data[i];
					//命令分析
        if(('\r' == ch) || ('\n' == ch)) {
            if(uart_cmd_len > 0U) {
                uart_cmd_buf[uart_cmd_len] = '\0';
                uart_process_command(uart_cmd_buf);
                uart_cmd_len = 0U;
                uart_cmd_buf[0] = '\0';
                uart_print_prompt();
            }
            continue;
        }
					//退格键，光标处理
//        if((0x08U == (uint8_t)ch) || (0x7FU == (uint8_t)ch)) {
//            if(uart_cmd_len > 0U) {
//                uart_cmd_len--;
//                uart_cmd_buf[uart_cmd_len] = '\0';
//                bsp_uart_dma_send((const uint8_t *)"\b \b", 3U);
//            }
//            continue;
//        }
				//忽略控制字符
//        if(((uint8_t)ch < 0x20U) || ((uint8_t)ch > 0x7EU)) {
//            continue;
//        }

        if(uart_cmd_len < (UART_CMD_BUF_SIZE - 1U)) {
            uart_cmd_buf[uart_cmd_len++] = ch;
            uart_cmd_buf[uart_cmd_len] = '\0';
            bsp_uart_dma_send((const uint8_t *)&ch, 1U);
        } else {
					//命令太长
            uart_cmd_len = 0U;
            uart_cmd_buf[0] = '\0';
            bsp_uart_dma_send((const uint8_t *)err, (uint16_t)strlen(err));
            bsp_uart_dma_send((const uint8_t *)prompt, (uint16_t)strlen(prompt));
        }
    }
}
