/*!
    \file    bsp_dac.c
    \brief   GD32 on-chip DAC output on PA4 (OUT0) and PA5 (OUT1)
*/

#include "bsp_dac.h"

#define BSP_DAC_VREF_MV    3300U

static uint16_t bsp_dac_mv_to_data(uint16_t mv)
{
    if(mv > BSP_DAC_VREF_MV) {
        mv = BSP_DAC_VREF_MV;
    }

    return (uint16_t)(((uint32_t)mv * 4095U) / BSP_DAC_VREF_MV);
}

void bsp_dac_init(void)
{
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_DAC);

    gpio_mode_set(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_PIN_4 | GPIO_PIN_5);

    dac_deinit(DAC0);
    dac_trigger_disable(DAC0, DAC_OUT0);
    dac_trigger_disable(DAC0, DAC_OUT1);
    dac_wave_mode_config(DAC0, DAC_OUT0, DAC_WAVE_DISABLE);
    dac_wave_mode_config(DAC0, DAC_OUT1, DAC_WAVE_DISABLE);
    dac_concurrent_enable(DAC0);
    bsp_dac_set_voltage_mv(1650U, 1650U);
}

void bsp_dac_set_voltage_mv(uint16_t out0_mv, uint16_t out1_mv)
{
    dac_concurrent_data_set(DAC0,
                            DAC_ALIGN_12B_R,
                            bsp_dac_mv_to_data(out0_mv),
                            bsp_dac_mv_to_data(out1_mv));
}

void bsp_dac_disable(void)
{
    dac_concurrent_disable(DAC0);
}
