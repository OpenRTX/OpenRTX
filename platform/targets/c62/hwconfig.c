/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <hwconfig.h>
#include <pthread.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/device.h>
#include "drivers/GPIO/gpio_zephyr.h"
#include "drivers/SPI/spi_zephyr.h"
#include "drivers/I2C/i2c_bitbang.h"
#include "drivers/audio/csk6_codec.h"

GPIO_ZEPHYR_DEVICE_DEFINE(GpioA, gpioa)
GPIO_ZEPHYR_DEVICE_DEFINE(GpioB, gpiob)

static pthread_mutex_t c62_bk4819_mutex;

static const struct spiZephyrCfg c62_bk4819_spiCfg = {
    .bus = DEVICE_DT_GET(DT_NODELABEL(bk4819_spi)),
    .frequency = 200000,
    .flags = SPI_HALF_DUPLEX,
};

SPI_ZEPHYR_DEVICE_DEFINE(c62_bk4819_spi, c62_bk4819_spiCfg, &c62_bk4819_mutex)

const struct BK4819 c62_bk4819 = {
    .spi = &c62_bk4819_spi,
    .scn = { BK4819_SCN },
};

GPIO_BK4819_DEVICE_DEFINE(c62_bk4819_gpio, c62_bk4819)

static struct i2cBitbangState c62_bk1080_i2cState;

static const struct i2cBitbangCfg c62_bk1080_i2cCfg = {
    .sck = { BK1080_SCK },
    .sda = { BK1080_SDA },
    .clkPeriod = 10,
    .state = &c62_bk1080_i2cState,
};

I2C_BITBANG_DEVICE_DEFINE(c62_bk1080_i2c, c62_bk1080_i2cCfg, NULL)

const struct BK1080 c62_bk1080 = {
    .i2c = &c62_bk1080_i2c,
    .pwr = { BK1080_PWR },
};

/*
 * The TX DAC sets the level into the BK4819 mic input for both FM and M17;
 * never above 0 dB.
 */
const struct csk6codecCfg c62_codecCfg = {
    .adcLeft = { .analog = 6, .digital = 10 },   // Microphone
    .adcRight = { .analog = 6, .digital = 20 },  // BK4819 RX audio
    .dacLeft = { .analog = -6, .digital = 0 },   // Speaker
    .dacRight = { .analog = -18, .digital = 0 }, // BK4819 TX audio
};

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
