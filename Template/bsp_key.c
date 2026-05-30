/*!
    \file    bsp_key.c
    \brief   USER/TAMPER/WAKEUP keys on GD32F450I-EVAL compatible pins
*/

#include "bsp_key.h"
#include "gd32f450i_eval.h"
#include "systick.h"

#define BSP_KEY_DEBOUNCE_MS    30U

static uint8_t bsp_key_is_pressed(key_typedef_enum key)
{
    return (RESET == gd_eval_key_state_get(key)) ? 1U : 0U;
}

void bsp_key_init(void)
{
    gd_eval_key_init(KEY_USER, KEY_MODE_GPIO);
    gd_eval_key_init(KEY_TAMPER, KEY_MODE_GPIO);
    gd_eval_key_init(KEY_WAKEUP, KEY_MODE_GPIO);
}

bsp_key_event_t bsp_key_poll(void)
{
    static uint8_t user_prev = 0U;
    static uint8_t tamper_prev = 0U;
    static uint8_t wakeup_prev = 0U;
    static uint32_t last_change_ms = 0U;
    uint8_t user_now;
    uint8_t tamper_now;
    uint8_t wakeup_now;
    uint32_t now;

    now = systick_ms_get();
    user_now = bsp_key_is_pressed(KEY_USER);
    tamper_now = bsp_key_is_pressed(KEY_TAMPER);
    wakeup_now = bsp_key_is_pressed(KEY_WAKEUP);

    if((now - last_change_ms) < BSP_KEY_DEBOUNCE_MS) {
        return BSP_KEY_NONE;
    }

    if((0U != user_now) && (0U == user_prev)) {
        user_prev = user_now;
        last_change_ms = now;
        return BSP_KEY_USER;
    }
    user_prev = user_now;

    if((0U != tamper_now) && (0U == tamper_prev)) {
        tamper_prev = tamper_now;
        last_change_ms = now;
        return BSP_KEY_TAMPER;
    }
    tamper_prev = tamper_now;

    if((0U != wakeup_now) && (0U == wakeup_prev)) {
        wakeup_prev = wakeup_now;
        last_change_ms = now;
        return BSP_KEY_WAKEUP;
    }
    wakeup_prev = wakeup_now;

    return BSP_KEY_NONE;
}
