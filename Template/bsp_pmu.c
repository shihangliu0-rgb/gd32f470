/*!
    \file    bsp_pmu.c
    \brief   Sleep (WFI) and deepsleep with EXTI wakeup
*/

#include "bsp_pmu.h"
#include "gd32f450i_eval.h"
#include "bsp_uart_dma.h"
#include "systick.h"
#include "gd32f4xx.h"

static volatile uint8_t pmu_wakeup_flag = 0U;

static void bsp_pmu_soft_delay(uint32_t time)
{
    volatile uint32_t i;

    for(i = 0U; i < (time * 10U); i++) {
    }
}

void bsp_pmu_init(void)
{
    rcu_periph_clock_enable(RCU_PMU);
    gd_eval_key_init(KEY_TAMPER, KEY_MODE_EXTI);
    gd_eval_key_init(KEY_WAKEUP, KEY_MODE_EXTI);
}

void bsp_pmu_enter_sleep(void)
{
    pmu_to_sleepmode(WFI_CMD);
}

void bsp_pmu_enter_deepsleep(void)
{
    bsp_pmu_soft_delay(0x50U);
    rcu_ahb_clock_config(RCU_AHB_CKSYS_DIV2);
    bsp_pmu_soft_delay(0x50U);
    rcu_ahb_clock_config(RCU_AHB_CKSYS_DIV4);
    bsp_pmu_soft_delay(0x50U);
    rcu_ahb_clock_config(RCU_AHB_CKSYS_DIV8);
    bsp_pmu_soft_delay(0x50U);
    rcu_ahb_clock_config(RCU_AHB_CKSYS_DIV16);
    bsp_pmu_soft_delay(0x50U);
    rcu_system_clock_source_config(RCU_CKSYSSRC_IRC16M);
    bsp_pmu_soft_delay(200U);
    rcu_ahb_clock_config(RCU_AHB_CKSYS_DIV1);

    pmu_to_deepsleepmode(PMU_LDO_LOWPOWER, PMU_LOWDRIVER_ENABLE, WFI_CMD);

    SystemInit();
    systick_config();
    gd_eval_com_init(EVAL_COM0);
    bsp_uart_dma_init();
    pmu_wakeup_flag = 1U;
}

uint8_t bsp_pmu_wakeup_pending(void)
{
    return pmu_wakeup_flag;
}

void bsp_pmu_clear_wakeup(void)
{
    pmu_wakeup_flag = 0U;
}

void bsp_pmu_on_exti_wakeup(void)
{
    pmu_wakeup_flag = 1U;
}
