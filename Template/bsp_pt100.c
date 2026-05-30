/*!
    \file    bsp_pt100.c
    \brief   PT100 temperature measurement via GD30AD3344 (SPI3)

    [驱动层] SPI + DRDY + DMA
    [采样层] 状态机 + 滤波 + 校验
    [应用层] PT100温度 + RTC + 上报
*/

#include "bsp_pt100.h"
#include "bsp_rtc.h"
#include "systick.h"
#include "gd32f4xx.h"
#include <math.h>
#include <stdio.h>

/* ========================================================================== */
/*  Board / analog front-end                                                  */
/* ========================================================================== */

#define PT100_SPI                     SPI3
#define PT100_SPI_CLK                 RCU_SPI3
#define PT100_SPI_GPIO_CLK            RCU_GPIOE
#define PT100_SPI_GPIO_PORT           GPIOE
#define PT100_SPI_SCK_PIN             GPIO_PIN_12
#define PT100_SPI_MISO_PIN            GPIO_PIN_13
#define PT100_SPI_MOSI_PIN            GPIO_PIN_14
#define PT100_SPI_AF                  GPIO_AF_5
#define PT100_SPI_NSS_PORT            GPIOE
#define PT100_SPI_NSS_PIN             GPIO_PIN_11

#define PT100_SPI_NSS_HIGH()          gpio_bit_set(PT100_SPI_NSS_PORT, PT100_SPI_NSS_PIN)
#define PT100_SPI_NSS_LOW()           gpio_bit_reset(PT100_SPI_NSS_PORT, PT100_SPI_NSS_PIN)

#define PT100_SPI_DATA_ADDRESS        ((uint32_t)&SPI_DATA(SPI3))
#define PT100_SPI_DMA_RX              DMA1
#define PT100_SPI_DMA_RX_CH           DMA_CH0
#define PT100_SPI_DMA_TX              DMA1
#define PT100_SPI_DMA_TX_CH           DMA_CH1
#define PT100_SPI_DMA_SUBPERI         DMA_SUBPERI4
#define PT100_SPI_DMA_WORDS           2U

#define GD30AD3344_CFG_MUX_AIN0_AIN1  (0x0U << 12)
#define GD30AD3344_CFG_PGA_2048MV     (0x2U << 9)//改这里的寄存器的配置，修改量程
#define GD30AD3344_CFG_MODE_CONT      (0x0U << 8)
#define GD30AD3344_CFG_DR_128SPS      (0x4U << 5)
#define GD30AD3344_CFG_PULLUP_EN      (0x1U << 3)
#define GD30AD3344_CFG_NOP_VALID      (0x1U << 1)
#define GD30AD3344_CFG_RESERVED       (0x1U)

#define GD30AD3344_FSR_V              2.048f //量程的值
#define GD30AD3344_CONV_TIMEOUT_MS    30U
#define GD30AD3344_SETTLE_MS          10U
#define PT100_ADC_AVG_COUNT           4U
#define PT100_ADC_DISCARD_COUNT       1U
#define PT100_RAW_RETRY_COUNT         3U    //最多重试3次

#define PT100_REF_V                   3.3f    //恒定的电压
#define PT100_EXCITATION_I_A          (PT100_REF_V / 3300.0f)//恒定的电流
#define PT100_AD623_RG_OHM            6000.0f   //增益电阻
#define PT100_AD623_GAIN              (1.0f + 100000.0f / PT100_AD623_RG_OHM)   //增益值 17.xx
#define PT100_VOLTAGE_TO_R_SCALE      (PT100_AD623_GAIN * PT100_EXCITATION_I_A)  

#define PT100_R0_OHM                  100.0f
#define PT100_CVD_A                   3.9083e-3f
#define PT100_CVD_B                   (-5.775e-7f)
#define PT100_CVD_C                   (-4.183e-12f)
#define PT100_NEWTON_ITER             12U

static const uint16_t gd30ad3344_config =
    GD30AD3344_CFG_MUX_AIN0_AIN1 |
    GD30AD3344_CFG_PGA_2048MV |
    GD30AD3344_CFG_MODE_CONT |
    GD30AD3344_CFG_DR_128SPS |
    GD30AD3344_CFG_PULLUP_EN |
    GD30AD3344_CFG_NOP_VALID |
    GD30AD3344_CFG_RESERVED;

/* ========================================================================== */
/*  [驱动层] SPI + DRDY + DMA                                                  */
/* ========================================================================== */

typedef enum {
    PT100_DRV_IDLE = 0,
    PT100_DRV_WAIT_DRDY,
    PT100_DRV_SPI_DMA,
    PT100_DRV_DONE,
    PT100_DRV_ERROR
} pt100_drv_state_t;

static uint8_t pt100_spi_tx_buf[PT100_SPI_DMA_WORDS] = {0x00U, 0x00U};
static uint8_t pt100_spi_rx_buf[PT100_SPI_DMA_WORDS];
static volatile uint8_t pt100_spi_dma_rx_done = 0U;
static volatile pt100_drv_state_t pt100_drv_state = PT100_DRV_IDLE;

static void pt100_drv_rcu_config(void);
static void pt100_drv_gpio_config(void);
static void pt100_drv_spi_config(void);
static void pt100_drv_nvic_config(void);
static void pt100_drv_dma_config(void);
static void pt100_drv_dma_reconfig(void);
static void pt100_drv_miso_as_gpio_input(void);
static void pt100_drv_miso_as_spi_af(void);
static void pt100_drv_spi_flush_rx(void);
static uint8_t pt100_drv_spi_transfer_byte(uint8_t data);
static void pt100_drv_write_config(uint16_t config);
static uint8_t pt100_drv_drdy_is_low(void);
static uint8_t pt100_drv_wait_drdy_level(uint8_t expect_low, uint32_t timeout_ms);
static int16_t pt100_drv_spi_dma_read_result(void);
static uint8_t pt100_drv_raw_looks_invalid(int16_t raw);
static int16_t pt100_drv_read_raw_once(void);

static void pt100_drv_rcu_config(void)
{
    rcu_periph_clock_enable(PT100_SPI_GPIO_CLK);
    rcu_periph_clock_enable(PT100_SPI_CLK);
    rcu_periph_clock_enable(RCU_DMA1);
}

static void pt100_drv_nvic_config(void)
{
    nvic_irq_enable(DMA1_Channel0_IRQn, 0U, 0U);
}

static void pt100_drv_dma_config(void)
{
    dma_single_data_parameter_struct dma_init_struct;

    dma_single_data_para_struct_init(&dma_init_struct);

    dma_deinit(PT100_SPI_DMA_RX, PT100_SPI_DMA_RX_CH);
    dma_init_struct.direction = DMA_PERIPH_TO_MEMORY;
    dma_init_struct.memory0_addr = (uint32_t)pt100_spi_rx_buf;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_8BIT;
    dma_init_struct.number = PT100_SPI_DMA_WORDS;
    dma_init_struct.periph_addr = PT100_SPI_DATA_ADDRESS;
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.priority = DMA_PRIORITY_HIGH;
	
    dma_single_data_mode_init(PT100_SPI_DMA_RX, PT100_SPI_DMA_RX_CH, &dma_init_struct);
    dma_circulation_disable(PT100_SPI_DMA_RX, PT100_SPI_DMA_RX_CH);
    dma_channel_subperipheral_select(PT100_SPI_DMA_RX, PT100_SPI_DMA_RX_CH, PT100_SPI_DMA_SUBPERI);
    dma_interrupt_enable(PT100_SPI_DMA_RX, PT100_SPI_DMA_RX_CH, DMA_CHXCTL_FTFIE);

    dma_deinit(PT100_SPI_DMA_TX, PT100_SPI_DMA_TX_CH);
    dma_init_struct.direction = DMA_MEMORY_TO_PERIPH;
    dma_init_struct.memory0_addr = (uint32_t)pt100_spi_tx_buf;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_8BIT;
    dma_init_struct.number = PT100_SPI_DMA_WORDS;
    dma_init_struct.periph_addr = PT100_SPI_DATA_ADDRESS;
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.priority = DMA_PRIORITY_HIGH;
		
    dma_single_data_mode_init(PT100_SPI_DMA_TX, PT100_SPI_DMA_TX_CH, &dma_init_struct);
    dma_circulation_disable(PT100_SPI_DMA_TX, PT100_SPI_DMA_TX_CH);
    dma_channel_subperipheral_select(PT100_SPI_DMA_TX, PT100_SPI_DMA_TX_CH, PT100_SPI_DMA_SUBPERI);
		//spi双工，所以只用开启一个中断，就行了
}

static void pt100_drv_dma_reconfig(void)
{
    pt100_spi_dma_rx_done = 0U;

    dma_channel_disable(PT100_SPI_DMA_RX, PT100_SPI_DMA_RX_CH);
    dma_channel_disable(PT100_SPI_DMA_TX, PT100_SPI_DMA_TX_CH);
    dma_interrupt_flag_clear(PT100_SPI_DMA_RX, PT100_SPI_DMA_RX_CH, DMA_INT_FLAG_FTF);
    dma_flag_clear(PT100_SPI_DMA_RX, PT100_SPI_DMA_RX_CH, DMA_FLAG_FTF);
    dma_flag_clear(PT100_SPI_DMA_TX, PT100_SPI_DMA_TX_CH, DMA_FLAG_FTF);

    dma_memory_address_config(PT100_SPI_DMA_RX, PT100_SPI_DMA_RX_CH, DMA_MEMORY_0, (uint32_t)pt100_spi_rx_buf);
    dma_memory_address_config(PT100_SPI_DMA_TX, PT100_SPI_DMA_TX_CH, DMA_MEMORY_0, (uint32_t)pt100_spi_tx_buf);
    dma_transfer_number_config(PT100_SPI_DMA_RX, PT100_SPI_DMA_RX_CH, PT100_SPI_DMA_WORDS);
    dma_transfer_number_config(PT100_SPI_DMA_TX, PT100_SPI_DMA_TX_CH, PT100_SPI_DMA_WORDS);
}

static void pt100_drv_gpio_config(void)
{
    gpio_af_set(PT100_SPI_GPIO_PORT, PT100_SPI_AF,
                PT100_SPI_SCK_PIN | PT100_SPI_MISO_PIN | PT100_SPI_MOSI_PIN);
    gpio_mode_set(PT100_SPI_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE,
                  PT100_SPI_SCK_PIN | PT100_SPI_MISO_PIN | PT100_SPI_MOSI_PIN);
    gpio_output_options_set(PT100_SPI_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ,
                            PT100_SPI_SCK_PIN | PT100_SPI_MISO_PIN | PT100_SPI_MOSI_PIN);

    gpio_mode_set(PT100_SPI_NSS_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, PT100_SPI_NSS_PIN);
    gpio_output_options_set(PT100_SPI_NSS_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, PT100_SPI_NSS_PIN);
    PT100_SPI_NSS_HIGH();

    pt100_drv_miso_as_gpio_input();
}

static void pt100_drv_spi_config(void)
{
    spi_parameter_struct spi_init_struct;

    spi_init_struct.trans_mode           = SPI_TRANSMODE_FULLDUPLEX;
    spi_init_struct.device_mode          = SPI_MASTER;
    spi_init_struct.frame_size           = SPI_FRAMESIZE_8BIT;
    spi_init_struct.clock_polarity_phase = SPI_CK_PL_LOW_PH_2EDGE;
    spi_init_struct.nss                  = SPI_NSS_SOFT;
    spi_init_struct.prescale             = SPI_PSC_128;
    spi_init_struct.endian               = SPI_ENDIAN_MSB;
    spi_init(PT100_SPI, &spi_init_struct);
    spi_enable(PT100_SPI);
}

static void pt100_drv_miso_as_gpio_input(void)
{
    gpio_mode_set(PT100_SPI_GPIO_PORT, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, PT100_SPI_MISO_PIN);
}

static void pt100_drv_miso_as_spi_af(void)
{
    gpio_af_set(PT100_SPI_GPIO_PORT, PT100_SPI_AF, PT100_SPI_MISO_PIN);
    gpio_mode_set(PT100_SPI_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, PT100_SPI_MISO_PIN);
    gpio_output_options_set(PT100_SPI_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, PT100_SPI_MISO_PIN);
}

static void pt100_drv_spi_flush_rx(void)
{
    while(SET == spi_i2s_flag_get(PT100_SPI, SPI_FLAG_RBNE)) {
        (void)spi_i2s_data_receive(PT100_SPI);
    }
}

static uint8_t pt100_drv_spi_transfer_byte(uint8_t data)
{
    while(RESET == spi_i2s_flag_get(PT100_SPI, SPI_FLAG_TBE)) {
    }
    spi_i2s_data_transmit(PT100_SPI, data);

    while(RESET == spi_i2s_flag_get(PT100_SPI, SPI_FLAG_RBNE)) {
    }

    return (uint8_t)spi_i2s_data_receive(PT100_SPI);
}

static void pt100_drv_write_config(uint16_t config)
{
    uint8_t config_bytes[2];

    config_bytes[0] = (uint8_t)(config >> 8);
    config_bytes[1] = (uint8_t)(config & 0xFFU);

    pt100_drv_miso_as_spi_af();
    pt100_drv_spi_flush_rx();
    PT100_SPI_NSS_LOW();
    
    //写两次，协议规定
    pt100_drv_spi_transfer_byte(config_bytes[0]);
    pt100_drv_spi_transfer_byte(config_bytes[1]);
    pt100_drv_spi_transfer_byte(config_bytes[0]);
    pt100_drv_spi_transfer_byte(config_bytes[1]);

    PT100_SPI_NSS_HIGH();
    pt100_drv_miso_as_gpio_input();
}
//高电平返回0，低电平返回1
static uint8_t pt100_drv_drdy_is_low(void)
{
    return (RESET == gpio_input_bit_get(PT100_SPI_GPIO_PORT, PT100_SPI_MISO_PIN)) ? 1U : 0U;
}
//超时保护机制
static uint8_t pt100_drv_wait_drdy_level(uint8_t expect_low, uint32_t timeout_ms)
{
    uint32_t start = systick_ms_get();

    while((systick_ms_get() - start) < timeout_ms) {
        if(expect_low == pt100_drv_drdy_is_low()) {
            return 1U;
        }
    }

    return 0U;
}

static uint8_t pt100_drv_raw_looks_invalid(int16_t raw)
{
    uint16_t u = (uint16_t)raw;

    if((0xFFU == (u & 0xFFU)) || (0x00U == (u & 0xFFU))) {
        return 1U;
    }

    return 0U;
}

static int16_t pt100_drv_spi_dma_read_result(void)
{
    uint32_t start;
    uint16_t raw;

    pt100_drv_state = PT100_DRV_SPI_DMA;
    pt100_drv_dma_reconfig();

    pt100_drv_miso_as_spi_af();
    pt100_drv_spi_flush_rx();
    PT100_SPI_NSS_LOW();

    spi_dma_enable(PT100_SPI, SPI_DMA_TRANSMIT);
    spi_dma_enable(PT100_SPI, SPI_DMA_RECEIVE);
    dma_channel_enable(PT100_SPI_DMA_TX, PT100_SPI_DMA_TX_CH);
    dma_channel_enable(PT100_SPI_DMA_RX, PT100_SPI_DMA_RX_CH);

    start = systick_ms_get();
    while((0U == pt100_spi_dma_rx_done) &&
          ((systick_ms_get() - start) < GD30AD3344_CONV_TIMEOUT_MS)) {
    }

    dma_channel_disable(PT100_SPI_DMA_RX, PT100_SPI_DMA_RX_CH);
    dma_channel_disable(PT100_SPI_DMA_TX, PT100_SPI_DMA_TX_CH);
    spi_dma_disable(PT100_SPI, SPI_DMA_RECEIVE);
    spi_dma_disable(PT100_SPI, SPI_DMA_TRANSMIT);

    PT100_SPI_NSS_HIGH();
    pt100_drv_miso_as_gpio_input();//等待下一次数据
		//dma已经关闭，如果现在还是0没完成的话
    if(0U == pt100_spi_dma_rx_done) {
        pt100_drv_state = PT100_DRV_ERROR;
        return 0;
    }

    raw = ((uint16_t)pt100_spi_rx_buf[0] << 8);
    raw |= (uint16_t)pt100_spi_rx_buf[1];
    pt100_drv_state = PT100_DRV_DONE;
    return (int16_t)raw;
}

static int16_t pt100_drv_read_raw_once(void)
{
    uint32_t retry;

    for(retry = 0U; retry < PT100_RAW_RETRY_COUNT; retry++) {
        int16_t raw;

        pt100_drv_state = PT100_DRV_WAIT_DRDY;

        if(0U != pt100_drv_drdy_is_low()) {
            (void)pt100_drv_spi_dma_read_result();
        }
				
        (void)pt100_drv_wait_drdy_level(0U, GD30AD3344_CONV_TIMEOUT_MS);
				//等待高电平
        if(0U == pt100_drv_wait_drdy_level(1U, GD30AD3344_CONV_TIMEOUT_MS)) {
            delay_1ms(GD30AD3344_SETTLE_MS);
        }

        raw = pt100_drv_spi_dma_read_result();
        if(0U == pt100_drv_raw_looks_invalid(raw)) {
            return raw;
        }
    }

    pt100_drv_state = PT100_DRV_ERROR;
    return 0;
}

void bsp_pt100_spi_dma_rx_over_flag(void)
{
    if(RESET != dma_interrupt_flag_get(PT100_SPI_DMA_RX, PT100_SPI_DMA_RX_CH, DMA_INT_FLAG_FTF)) {
        dma_interrupt_flag_clear(PT100_SPI_DMA_RX, PT100_SPI_DMA_RX_CH, DMA_INT_FLAG_FTF);
        pt100_spi_dma_rx_done = 1U;
    }
}

/* ========================================================================== */
/*  [采样层] 状态机 + 滤波 + 校验                                              */
/* ========================================================================== */

typedef enum {
    PT100_SMP_IDLE = 0,
    PT100_SMP_DISCARD,
    PT100_SMP_COLLECT,
    PT100_SMP_FILTER,
    PT100_SMP_READY,
    PT100_SMP_ERROR
} pt100_smp_state_t;

static pt100_smp_state_t pt100_smp_state = PT100_SMP_IDLE;

static uint8_t pt100_smp_raw_looks_invalid(int16_t raw)
{
    return pt100_drv_raw_looks_invalid(raw);
}
//舍弃首次采样，加4次采样的均值
static int16_t pt100_smp_acquire_filtered(void)
{
    uint32_t i;
    int32_t sum = 0;
    uint32_t valid = 0U;

    pt100_smp_state = PT100_SMP_DISCARD;
    for(i = 0U; i < PT100_ADC_DISCARD_COUNT; i++) {
        (void)pt100_drv_read_raw_once();
    }

    pt100_smp_state = PT100_SMP_COLLECT;
    for(i = 0U; i < PT100_ADC_AVG_COUNT; i++) {
        int16_t sample = pt100_drv_read_raw_once();

        if(0U == pt100_smp_raw_looks_invalid(sample)) {
            sum += (int32_t)sample;
            valid++;
        }
    }

    pt100_smp_state = PT100_SMP_FILTER;
    if(0U == valid) {
        pt100_smp_state = PT100_SMP_ERROR;
        return 0;
    }

    pt100_smp_state = PT100_SMP_READY;
    return (int16_t)(sum / (int32_t)valid);
}

/* ========================================================================== */
/*  [应用层] PT100温度 + RTC + 上报                                              */
/* ========================================================================== */

static float pt100_app_raw_to_voltage(int16_t raw)
{
    return ((float)raw * GD30AD3344_FSR_V) / 32768.0f;
}

static float pt100_app_voltage_to_resistance(float voltage)
{
    if(PT100_VOLTAGE_TO_R_SCALE <= 0.0f) {
        return 0.0f;
    }

    return voltage / PT100_VOLTAGE_TO_R_SCALE;
}

static float pt100_app_resistance_to_temperature(float resistance)
{
    float ratio;
    float discriminant;
    float temperature;
    uint32_t i;

    if(resistance <= 0.0f) {
        return -200.0f;
    }

    ratio = resistance / PT100_R0_OHM - 1.0f;

    if(resistance >= PT100_R0_OHM) {
        discriminant = PT100_CVD_A * PT100_CVD_A + 4.0f * PT100_CVD_B * ratio;
        if(discriminant < 0.0f) {
            return 0.0f;
        }

        temperature = (-PT100_CVD_A + sqrtf(discriminant)) / (2.0f * PT100_CVD_B);
        if(temperature > 850.0f) {
            temperature = 850.0f;
        }
        return temperature;
    }

    temperature = ratio / PT100_CVD_A;
    for(i = 0U; i < PT100_NEWTON_ITER; i++) {
        float t2 = temperature * temperature;
        float t3 = t2 * temperature;
        float rt_est;
        float drt_dt;

        rt_est = PT100_R0_OHM * (1.0f + PT100_CVD_A * temperature + PT100_CVD_B * t2 +
                                 PT100_CVD_C * (temperature - 100.0f) * t3);
        drt_dt = PT100_R0_OHM * (PT100_CVD_A + 2.0f * PT100_CVD_B * temperature +
                                 PT100_CVD_C * (4.0f * t3 - 300.0f * t2));
        if((drt_dt > -1e-9f) && (drt_dt < 1e-9f)) {
            break;
        }

        temperature -= (rt_est - resistance) / drt_dt;
    }

    if(temperature < -200.0f) {
        temperature = -200.0f;
    }

    return temperature;
}

static void pt100_app_fill_rtc(adc_sample_t *sample)
{
    bsp_rtc_datetime_t datetime;

    bsp_rtc_get_datetime(&datetime);
    sample->year = datetime.year;
    sample->month = datetime.month;
    sample->day = datetime.day;
    sample->hour = datetime.hour;
    sample->minute = datetime.minute;
    sample->second = datetime.second;
}

void pt100_init(void)
{
    pt100_drv_state = PT100_DRV_IDLE;
    pt100_smp_state = PT100_SMP_IDLE;

    pt100_drv_rcu_config();
    pt100_drv_nvic_config();
    pt100_drv_dma_config();
    pt100_drv_gpio_config();
    pt100_drv_spi_config();

    PT100_SPI_NSS_HIGH();
    delay_1ms(1U);

    pt100_drv_write_config(gd30ad3344_config);
    delay_1ms(10U);

    (void)pt100_drv_read_raw_once();
    (void)pt100_drv_read_raw_once();
}

void pt100_measure_sample(adc_sample_t *sample)
{
    int16_t adc_raw;
    float adc_voltage;
    float pt100_resistance;
    float pt100_temperature;

    if(NULL == sample) {
        return;
    }

    adc_raw = pt100_smp_acquire_filtered();
    adc_voltage = pt100_app_raw_to_voltage(adc_raw);
    pt100_resistance = pt100_app_voltage_to_resistance(adc_voltage);
    pt100_temperature = pt100_app_resistance_to_temperature(pt100_resistance);
    pt100_app_fill_rtc(sample);

    sample->raw = adc_raw;
    sample->voltage = adc_voltage;
    sample->resistance = pt100_resistance;
    sample->temperature = pt100_temperature;
}

void pt100_print_sample_line(uint32_t index, const adc_sample_t *sample)
{
    char time_text[24];
    bsp_rtc_datetime_t dt;

    if(NULL == sample) {
        return;
    }

    dt.year = sample->year;
    dt.month = sample->month;
    dt.day = sample->day;
    dt.hour = sample->hour;
    dt.minute = sample->minute;
    dt.second = sample->second;
    bsp_rtc_format(&dt, time_text, sizeof(time_text));

    printf("[%lu] %s raw=0x%04X (%d), V=%.4f, R=%.2f ohm, T=%.2f C",
           (unsigned long)index,
           time_text,
           (uint16_t)sample->raw,
           sample->raw,
           sample->voltage,
           sample->resistance,
           sample->temperature);

    if((0 == sample->raw) || (0U != pt100_smp_raw_looks_invalid(sample->raw))) {
        printf(" [ADC invalid]");
    }
    printf("\r\n");
}
