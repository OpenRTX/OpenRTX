/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <hwconfig.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/device.h>
#include "drivers/GPIO/gpio_zephyr.h"
#include "drivers/SPI/spi_zephyr.h"
#include "drivers/I2C/i2c_bitbang.h"

/*
 * Pin mapping, must match platform/targets/c62/c62.dts:
 * BK4819: SCLK gpioa 13, SDATA gpioa 7, SCN gpioa 8 (active low)
 * BK1080: SCLK gpioa 5, SDATA gpioa 6, PWR gpiob 2 (active low)
 */
static const struct spiZephyrCfg c62_bk4819_spiCfg = {
    .bus = DEVICE_DT_GET(DT_NODELABEL(bk4819_spi)),
    .frequency = 200000,
    .flags = SPI_HALF_DUPLEX
};

SPI_ZEPHYR_DEVICE_DEFINE(c62_bk4819_spi, c62_bk4819_spiCfg, NULL)

const struct BK4819 c62_bk4819 = { .spi = &c62_bk4819_spi,
                                   .scn = { &GpioA, 8 } };

static struct i2cBitbangState c62_bk1080_i2cState;

static const struct i2cBitbangCfg c62_bk1080_i2cCfg = {
    .sck = { &GpioA, 5 },
    .sda = { &GpioA, 6 },
    .clkPeriod = 10,
    .state = &c62_bk1080_i2cState,
};

I2C_BITBANG_DEVICE_DEFINE(c62_bk1080_i2c, c62_bk1080_i2cCfg, NULL)

const struct BK1080 c62_bk1080 = { .i2c = &c62_bk1080_i2c,
                                   .pwr = { &GpioB, 2 } };

const struct gpio_dt_spec speaker_enable =
    GPIO_DT_SPEC_GET(DT_PATH(gpio_controls, speaker_enable), gpios);
const struct gpio_dt_spec dtmf_enable =
    GPIO_DT_SPEC_GET(DT_PATH(gpio_controls, dtmf_enable), gpios);
const struct gpio_dt_spec button_ptt =
    GPIO_DT_SPEC_GET_OR(DT_NODELABEL(button_ptt), gpios, { 0 });
const struct gpio_dt_spec led_white = GPIO_DT_SPEC_GET(DT_ALIAS(ledwhite),
                                                       gpios);
const struct gpio_dt_spec led_green = GPIO_DT_SPEC_GET(DT_ALIAS(ledgreen),
                                                       gpios);
const struct gpio_dt_spec led_keyboard =
    GPIO_DT_SPEC_GET(DT_NODELABEL(ledkeyboard), gpios);

const struct pwm_dt_spec pwm_lcd_backlight =
    PWM_DT_SPEC_GET(DT_NODELABEL(pwm_lcd_backlight));
const struct pwm_dt_spec pwm_rf_apc = PWM_DT_SPEC_GET(DT_NODELABEL(pwm_rf_apc));

const struct device *adc_dev = DEVICE_DT_GET(DT_NODELABEL(adc0));
