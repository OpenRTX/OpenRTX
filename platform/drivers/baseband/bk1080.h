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
    BK1080_FLAG_STC = (1 << 14),
    BK1080_FLAG_SFBL = (1 << 13),
    BK1080_FLAG_AFCRL = (1 << 12),
    BK1080_FLAG_STEN = (1 << 9),
    BK1080_FLAG_ST = (1 << 8)
} bk1080_flag_t;

typedef enum
{
    I2C_ACK  = 0,
    I2C_NACK = 1
} iic_ack_t;

static const uint16_t BK1080_RegisterTable[] = {
    0x0008, 0x1080, 0x0201, 0x0000, 0x40C0, 0x0A1F, 0x002E, 0x02FF, 0x5B11,
    0x0000, 0x411E, 0x0000, 0xCE00, 0x0000, 0x0000, 0x1000, 0x3197, 0x0000,
    0x13FF, 0x9852, 0x0000, 0x0000, 0x0008, 0x0000, 0x51E1, 0xA8BC, 0x2645,
    0x00E4, 0x1CD8, 0x3A50, 0xEAE0, 0x3000, 0x0200, 0x0000,
};

typedef void(*seek_tune_complete_cb)(uint8_t status, uint8_t rssi, uint16_t channel);

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
 * @brief Set frequency
 * 
 */
void bk1080_set_frequency(uint16_t freq);

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
void bk1080_set_channel(uint16_t channel);

/**
 * @brief Seek channel by hardware
 *
 * @param mode Seek mode
 *              0: continue to seek when reach the boundary
 *              1: stop seek when reach the boundary
 *
 * @param dir Seek direction
 *              0: Down
 *              1: Up
 *
 * @param startChannel Start seeking channel
 * 
 * @param timeout Seek timeout
 *
 * @param seek_tune_complete_cb Seek or tune complete callback
 *                              param:
 *                                      status: 0: successful
 *                                              1: failed
 *                                      rssi:   rssi value
 *                                      channel: channel value
 *
 */
void bk1080_seek_channel_hw(uint8_t mode,
                            uint8_t dir,
                            uint16_t startChannel,
                            uint8_t timeout,
                            seek_tune_complete_cb cb);

/**
 * @brief Set band range and spacing
 *
 * @param band band range
 *              0:87.5-108MHz
 *              1:76-108MHz
 *              3:76-90MHz
 *
 * @param spacing search spacing
 *              0:200KHz
 *              1:100KHz
 *              3:50Khz
 */
void bk1080_set_band_spacing(uint8_t band, uint8_t spacing);

/**
 * @brief Get flag
 * 
 * @param flag flag type
 * @return uint8_t 
 */
uint8_t bk1080_get_flag(bk1080_flag_t flag);

#endif

#ifdef __cplusplus
}
#endif
