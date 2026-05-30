/*!
    \file    bsp_ext_flash.c
    \brief   GD25Q SPI NOR on SPI5 (PG12~14/PG13 SCK, PI8 CS) - GD32F450I-EVAL
*/

#include "bsp_ext_flash.h"
#include "systick.h"

#define EXT_FLASH_SPI              SPI5
#define EXT_FLASH_SPI_CLK          RCU_SPI5
#define EXT_FLASH_GPIO_CLK         RCU_GPIOG
#define EXT_FLASH_CS_GPIO_CLK      RCU_GPIOI
#define EXT_FLASH_GPIO_PORT        GPIOG
#define EXT_FLASH_CS_PORT          GPIOI
#define EXT_FLASH_SCK_PIN          GPIO_PIN_13
#define EXT_FLASH_MISO_PIN         GPIO_PIN_12
#define EXT_FLASH_MOSI_PIN         GPIO_PIN_14
#define EXT_FLASH_CS_PIN           GPIO_PIN_8
#define EXT_FLASH_AF               GPIO_AF_5

#define EXT_FLASH_CS_HIGH()        gpio_bit_set(EXT_FLASH_CS_PORT, EXT_FLASH_CS_PIN)
#define EXT_FLASH_CS_LOW()         gpio_bit_reset(EXT_FLASH_CS_PORT, EXT_FLASH_CS_PIN)

#define CMD_WRITE_ENABLE           0x06U
#define CMD_READ_STATUS            0x05U
#define CMD_READ_DATA              0x03U
#define CMD_PAGE_PROGRAM           0x02U
#define CMD_SECTOR_ERASE           0x20U
#define CMD_READ_ID                0x9FU
#define CMD_DUMMY                  0xA5U
#define STATUS_WIP                 0x01U
#define EXT_FLASH_SPI_TIMEOUT_MS   5U

static uint8_t ext_flash_ready = 0U;

static int8_t ext_flash_xfer_byte(uint8_t byte, uint8_t *rx)
{
    uint32_t start = systick_ms_get();

    while(RESET == spi_i2s_flag_get(EXT_FLASH_SPI, SPI_FLAG_TBE)) {
        if((systick_ms_get() - start) >= EXT_FLASH_SPI_TIMEOUT_MS) {
            return -1;
        }
    }
    spi_i2s_data_transmit(EXT_FLASH_SPI, byte);

    start = systick_ms_get();
    while(RESET == spi_i2s_flag_get(EXT_FLASH_SPI, SPI_FLAG_RBNE)) {
        if((systick_ms_get() - start) >= EXT_FLASH_SPI_TIMEOUT_MS) {
            return -1;
        }
    }

    if(NULL != rx) {
        *rx = (uint8_t)spi_i2s_data_receive(EXT_FLASH_SPI);
    } else {
        (void)spi_i2s_data_receive(EXT_FLASH_SPI);
    }

    return 0;
}

static int8_t ext_flash_write_enable(void)
{
    uint8_t dummy;

    EXT_FLASH_CS_LOW();
    if(0 != ext_flash_xfer_byte(CMD_WRITE_ENABLE, &dummy)) {
        EXT_FLASH_CS_HIGH();
        return -1;
    }
    EXT_FLASH_CS_HIGH();
    return 0;
}

static int8_t ext_flash_wait_busy(void)
{
    uint8_t status;
    uint8_t dummy;
    uint32_t retry;

    for(retry = 0U; retry < 1000U; retry++) {
        EXT_FLASH_CS_LOW();
        if(0 != ext_flash_xfer_byte(CMD_READ_STATUS, &dummy)) {
            EXT_FLASH_CS_HIGH();
            return -1;
        }
        if(0 != ext_flash_xfer_byte(CMD_DUMMY, &status)) {
            EXT_FLASH_CS_HIGH();
            return -1;
        }
        EXT_FLASH_CS_HIGH();

        if(0U == (status & STATUS_WIP)) {
            return 0;
        }
    }

    return -1;
}

int8_t bsp_ext_flash_init(void)
{
    spi_parameter_struct spi_init_struct;
    uint32_t id;

    rcu_periph_clock_enable(EXT_FLASH_GPIO_CLK);
    rcu_periph_clock_enable(EXT_FLASH_CS_GPIO_CLK);
    rcu_periph_clock_enable(EXT_FLASH_SPI_CLK);

    gpio_af_set(EXT_FLASH_GPIO_PORT, EXT_FLASH_AF,
                EXT_FLASH_SCK_PIN | EXT_FLASH_MISO_PIN | EXT_FLASH_MOSI_PIN);
    gpio_mode_set(EXT_FLASH_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE,
                  EXT_FLASH_SCK_PIN | EXT_FLASH_MISO_PIN | EXT_FLASH_MOSI_PIN);
    gpio_output_options_set(EXT_FLASH_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ,
                            EXT_FLASH_SCK_PIN | EXT_FLASH_MISO_PIN | EXT_FLASH_MOSI_PIN);

    gpio_mode_set(EXT_FLASH_CS_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, EXT_FLASH_CS_PIN);
    gpio_output_options_set(EXT_FLASH_CS_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, EXT_FLASH_CS_PIN);
    EXT_FLASH_CS_HIGH();

    spi_init_struct.trans_mode           = SPI_TRANSMODE_FULLDUPLEX;
    spi_init_struct.device_mode          = SPI_MASTER;
    spi_init_struct.frame_size           = SPI_FRAMESIZE_8BIT;
    spi_init_struct.clock_polarity_phase = SPI_CK_PL_LOW_PH_1EDGE;
    spi_init_struct.nss                  = SPI_NSS_SOFT;
    spi_init_struct.prescale             = SPI_PSC_32;
    spi_init_struct.endian               = SPI_ENDIAN_MSB;
    spi_init(EXT_FLASH_SPI, &spi_init_struct);
    spi_enable(EXT_FLASH_SPI);

    id = bsp_ext_flash_read_id();
    if((0U == id) || (0xFFFFFFU == (id & 0xFFFFFFU))) {
        ext_flash_ready = 0U;
        return -1;
    }

    ext_flash_ready = 1U;
    return 0;
}

uint32_t bsp_ext_flash_read_id(void)
{
    uint32_t id = 0U;
    uint8_t b0;
    uint8_t b1;
    uint8_t b2;

    EXT_FLASH_CS_LOW();
    if(0 != ext_flash_xfer_byte(CMD_READ_ID, &b0)) {
        EXT_FLASH_CS_HIGH();
        return 0U;
    }
    if(0 != ext_flash_xfer_byte(CMD_DUMMY, &b0)) {
        EXT_FLASH_CS_HIGH();
        return 0U;
    }
    if(0 != ext_flash_xfer_byte(CMD_DUMMY, &b1)) {
        EXT_FLASH_CS_HIGH();
        return 0U;
    }
    if(0 != ext_flash_xfer_byte(CMD_DUMMY, &b2)) {
        EXT_FLASH_CS_HIGH();
        return 0U;
    }
    EXT_FLASH_CS_HIGH();

    id = ((uint32_t)b0 << 16) | ((uint32_t)b1 << 8) | (uint32_t)b2;
    return id;
}

int8_t bsp_ext_flash_read(uint32_t addr, uint8_t *buf, uint32_t len)
{
    uint32_t i;

    if((0U == ext_flash_ready) || (NULL == buf) || (0U == len)) {
        return -1;
    }

    EXT_FLASH_CS_LOW();
    if(0 != ext_flash_xfer_byte(CMD_READ_DATA, NULL)) {
        EXT_FLASH_CS_HIGH();
        return -1;
    }
    if(0 != ext_flash_xfer_byte((uint8_t)((addr >> 16) & 0xFFU), NULL)) {
        EXT_FLASH_CS_HIGH();
        return -1;
    }
    if(0 != ext_flash_xfer_byte((uint8_t)((addr >> 8) & 0xFFU), NULL)) {
        EXT_FLASH_CS_HIGH();
        return -1;
    }
    if(0 != ext_flash_xfer_byte((uint8_t)(addr & 0xFFU), NULL)) {
        EXT_FLASH_CS_HIGH();
        return -1;
    }

    for(i = 0U; i < len; i++) {
        if(0 != ext_flash_xfer_byte(CMD_DUMMY, &buf[i])) {
            EXT_FLASH_CS_HIGH();
            return -1;
        }
    }

    EXT_FLASH_CS_HIGH();
    return 0;
}

int8_t bsp_ext_flash_write(uint32_t addr, const uint8_t *buf, uint32_t len)
{
    uint32_t i;

    if((0U == ext_flash_ready) || (NULL == buf) || (0U == len) ||
       (len > EXT_FLASH_PAGE_SIZE) ||
       (0U != (addr % EXT_FLASH_PAGE_SIZE))) {
        return -1;
    }

    if(0 != ext_flash_write_enable()) {
        return -1;
    }

    EXT_FLASH_CS_LOW();
    if(0 != ext_flash_xfer_byte(CMD_PAGE_PROGRAM, NULL)) {
        EXT_FLASH_CS_HIGH();
        return -1;
    }
    if(0 != ext_flash_xfer_byte((uint8_t)((addr >> 16) & 0xFFU), NULL)) {
        EXT_FLASH_CS_HIGH();
        return -1;
    }
    if(0 != ext_flash_xfer_byte((uint8_t)((addr >> 8) & 0xFFU), NULL)) {
        EXT_FLASH_CS_HIGH();
        return -1;
    }
    if(0 != ext_flash_xfer_byte((uint8_t)(addr & 0xFFU), NULL)) {
        EXT_FLASH_CS_HIGH();
        return -1;
    }

    for(i = 0U; i < len; i++) {
        if(0 != ext_flash_xfer_byte(buf[i], NULL)) {
            EXT_FLASH_CS_HIGH();
            return -1;
        }
    }

    EXT_FLASH_CS_HIGH();
    return ext_flash_wait_busy();
}

int8_t bsp_ext_flash_sector_erase(uint32_t addr)
{
    if(0U == ext_flash_ready) {
        return -1;
    }

    if(0 != ext_flash_write_enable()) {
        return -1;
    }

    EXT_FLASH_CS_LOW();
    if(0 != ext_flash_xfer_byte(CMD_SECTOR_ERASE, NULL)) {
        EXT_FLASH_CS_HIGH();
        return -1;
    }
    if(0 != ext_flash_xfer_byte((uint8_t)((addr >> 16) & 0xFFU), NULL)) {
        EXT_FLASH_CS_HIGH();
        return -1;
    }
    if(0 != ext_flash_xfer_byte((uint8_t)((addr >> 8) & 0xFFU), NULL)) {
        EXT_FLASH_CS_HIGH();
        return -1;
    }
    if(0 != ext_flash_xfer_byte((uint8_t)(addr & 0xFFU), NULL)) {
        EXT_FLASH_CS_HIGH();
        return -1;
    }
    EXT_FLASH_CS_HIGH();
    return ext_flash_wait_busy();
}

uint8_t bsp_ext_flash_is_ready(void)
{
    return ext_flash_ready;
}
