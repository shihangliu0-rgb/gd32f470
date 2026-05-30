/*!
    \file    bsp_pmu.h
    \brief   GD32 PMU sleep / deepsleep
*/

#ifndef BSP_PMU_H
#define BSP_PMU_H

#include "gd32f4xx.h"

void bsp_pmu_init(void);
void bsp_pmu_enter_sleep(void);
void bsp_pmu_enter_deepsleep(void);
uint8_t bsp_pmu_wakeup_pending(void);
void bsp_pmu_clear_wakeup(void);
void bsp_pmu_on_exti_wakeup(void);

#endif /* BSP_PMU_H */
