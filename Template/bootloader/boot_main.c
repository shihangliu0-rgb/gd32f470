/*!
    \file    boot_main.c
    \brief   Minimal GD32F470 bootloader (build as separate Keil target)

    Flash layout (512KB):
      0x08000000 - 0x08007FFF : Bootloader (32KB, sector 0~1)
      0x08008000 - 0x0807FFFF : Application

    Relocate the application link address to BSP_BOOT_APP_ADDR before use.
*/

#include "gd32f4xx.h"
#include "gd32f450i_eval.h"
#include "bsp_boot.h"
#include <stdio.h>

#define BOOT_WAIT_MS    2000U

static void boot_uart_init(void)
{
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_USART0);

    gpio_af_set(GPIOA, GPIO_AF_7, GPIO_PIN_9 | GPIO_PIN_10);
    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_9 | GPIO_PIN_10);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_9 | GPIO_PIN_10);

    usart_deinit(USART0);
    usart_baudrate_set(USART0, 115200U);
    usart_receive_config(USART0, USART_RECEIVE_ENABLE);
    usart_transmit_config(USART0, USART_TRANSMIT_ENABLE);
    usart_enable(USART0);
}

int main(void)
{
    uint32_t start;

    SystemInit();
    boot_uart_init();
    gd_eval_led_init(LED1);

    printf("\r\nGD32F470 Bootloader v1.0\r\n");

    if(0U != bsp_boot_iap_mode_active()) {
        printf("IAP flag set, stay in bootloader.\r\n");
        gd_eval_led_on(LED1);
        while(1) {
        }
    }

    if(0U != bsp_boot_app_is_valid(BSP_BOOT_APP_ADDR)) {
        printf("Jump to app @0x%08lX\r\n", (unsigned long)BSP_BOOT_APP_ADDR);
        bsp_boot_jump_to_app(BSP_BOOT_APP_ADDR);
    }

    printf("No valid app, wait %u ms...\r\n", (unsigned int)BOOT_WAIT_MS);
    start = 0U;
    while(start < (BOOT_WAIT_MS * 10000U)) {
        start++;
    }

    if(0U != bsp_boot_app_is_valid(BSP_BOOT_APP_ADDR)) {
        bsp_boot_jump_to_app(BSP_BOOT_APP_ADDR);
    }

    gd_eval_led_on(LED1);
    printf("Bootloader idle.\r\n");
    while(1) {
    }
}

#if defined(__GNUC__) && !defined(__clang__)
int __io_putchar(int ch)
{
    usart_data_transmit(USART0, (uint8_t)ch);
    while(RESET == usart_flag_get(USART0, USART_FLAG_TBE)) {
    }
    return ch;
}
#else
int fputc(int ch, FILE *f)
{
    (void)f;
    usart_data_transmit(USART0, (uint8_t)ch);
    while(RESET == usart_flag_get(USART0, USART_FLAG_TBE)) {
    }
    return ch;
}
#endif
