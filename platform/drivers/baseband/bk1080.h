#ifndef __BK1080_JAMIEXU_H__
#define __BK1080_JAMIEXU_H__
#include "gpio.h"
#include "interfaces/delays.h"
#include "peripherals/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BK1080_SCK_LOW gpio_clearPin(BK1080_CLK)
#define BK1080_SCK_HIGH gpio_setPin(BK1080_CLK)

#define BK1080_SDA_LOW gpio_clearPin(BK1080_DAT)
#define BK1080_SDA_HIGH gpio_setPin(BK1080_DAT)

#define BK1080_SDA_READ gpio_readPin(BK1080_DAT)
#define BK1080_SDA_DIR_IN gpio_setMode(BK1080_DAT, INPUT)
#define BK1080_SDA_DIR_OUT gpio_setMode(BK1080_DAT, OUTPUT)

#define BK1080_ADDRESS 0x80

typedef enum
{
    BK1080_REG_00 = 0X00,
    BK1080_REG_01 = 0X01,
    BK1080_REG_02 = 0X02,
    BK1080_REG_03 = 0X03,
    BK1080_REG_04 = 0X04,
    BK1080_REG_05 = 0X05,
    BK1080_REG_06 = 0X06,
    BK1080_REG_07 = 0X07,
    BK1080_REG_08 = 0X08,
    BK1080_REG_09 = 0X09,
    BK1080_REG_10 = 0X0A,
    BK1080_REG_11 = 0X0B
} bk1080_reg_t;

typedef enum
{
    I2C_ACK  = 0,
    I2C_NACK = 1
} iic_ack_t;

static const uint16_t BK1080_RegisterTable[] = {
    0x0008, 0x1080, 0x0201, 0x0000, 0x40C0, 0x0A1F, 0x002E, 0x02FF,
	0x5B11, 0x0000, 0x411E, 0x0000, 0xCE00, 0x0000, 0x0000, 0x1000,
	0x3197, 0x0000, 0x13FF, 0x9852, 0x0000, 0x0000, 0x0008, 0x0000,
	0x51E1, 0xA8BC, 0x2645, 0x00E4, 0x1CD8, 0x3A50, 0xEAE0, 0x3000,
	0x0200, 0x0000,
};

static void i2c_start(void);
static void i2c_stop(void);
static void i2c_write_byte(uint8_t data);
static uint8_t i2c_read_byte(void);
static void i2c_send_ack(iic_ack_t ack);
static uint8_t i2c_get_ack(void);
static void bk1080_delay(uint32_t count);

static void bk1080_write_reg(bk1080_reg_t reg, uint16_t data);
static uint16_t bk1080_read_reg(bk1080_reg_t reg);

/**
 * @brief Init sequence
 * 
 */
void bk1080_init(void);

/**
 * @brief Convert channel to frequency
 * 
 * @param channel 
 * @return uint16_t 
 */
uint16_t bk1080_chan_to_freq(uint16_t channel);

/**
 * @brief Convert frequency to channel
 * 
 * @param freq 
 * @return uint16_t 
 */
uint16_t bk1080_freq_to_chan(uint16_t freq);

/**
 * @brief Set Frequency to play
 * 
 * @param freq 
 */
void bk1080_set_frequency(uint16_t freq);

#endif

#ifdef __cplusplus
}
#endif
