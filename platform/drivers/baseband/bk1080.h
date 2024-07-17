#ifndef __BK1080_JAMIEXU_H__
#define __BK1080_JAMIEXU_H__
#include "gpio.h"
#include "peripherals/gpio.h"
#include "interfaces/delays.h"

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
    BK1080_REG00 = 0X00,
    BK1080_REG01 = 0X01,
    BK1080_REG02 = 0X02,
    BK1080_REG03 = 0X03,
    BK1080_REG04 = 0X04,
    BK1080_REG05 = 0X05,
    BK1080_REG06 = 0X06,
    BK1080_REG07 = 0X07,
    BK1080_REG08 = 0X08,
    BK1080_REG09 = 0X09,
    BK1080_REG10 = 0X0A,
    BK1080_REG11 = 0X0B
} bk1080_reg_t;

typedef enum
{
    I2C_ACK = 0,
    I2C_NACK = 1
} iic_ack_t;

static void i2c_start(void);
static void i2c_stop(void);
static void i2c_write_byte(uint8_t data);
static uint8_t i2c_read_byte(void);
static void i2c_send_ack(iic_ack_t ack);
static uint8_t i2c_get_ack(void);
static void bk1080_delay(uint32_t count);

void bk1080_write_reg(bk1080_reg_t reg, uint16_t data);
uint16_t bk1080_read_reg(bk1080_reg_t reg);

#endif

#ifdef __cplusplus
}
#endif
