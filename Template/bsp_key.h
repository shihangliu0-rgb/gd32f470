/*!
    \file    bsp_key.h
    \brief   GPIO button input with debounce
*/

#ifndef BSP_KEY_H
#define BSP_KEY_H

#include "gd32f4xx.h"

typedef enum {
    BSP_KEY_NONE = 0,
    BSP_KEY_USER,
    BSP_KEY_TAMPER,
    BSP_KEY_WAKEUP
} bsp_key_event_t;

void bsp_key_init(void);
bsp_key_event_t bsp_key_poll(void);

#endif /* BSP_KEY_H */
