/*!
    \file    app_cmd.h
    \brief   UART command line interface
*/

#ifndef APP_CMD_H
#define APP_CMD_H

#include <stdint.h>

#define UART_CMD_BUF_SIZE    96U

extern char uart_cmd_buf[UART_CMD_BUF_SIZE];
extern uint8_t uart_cmd_len;

void uart_process_command(char *cmd);
void uart_print_prompt(void);

#endif /* APP_CMD_H */
