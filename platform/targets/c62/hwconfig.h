/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef HWCONFIG_H
#define HWCONFIG_H

#define TARGET_C62

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "drivers/baseband/BK1080.h"
#include "drivers/baseband/BK4819.h"

/*
 * Display properties are encoded in the devicetree
 */
#define DISPLAY DT_CHOSEN(zephyr_display)
#define CONFIG_SCREEN_WIDTH DT_PROP(DISPLAY, width)
#define CONFIG_SCREEN_HEIGHT DT_PROP(DISPLAY, height)

#define CONFIG_PIX_FMT_RGB565

/* Screen has adjustable brightness */
#define CONFIG_SCREEN_BRIGHTNESS

/* Battery type */
#define CONFIG_BAT_LIION
#define CONFIG_BAT_NCELLS 2

#define CONFIG_M17

/* Microphone audio input, set for C62_ADC_MIC_GAIN_D below */
#define CONFIG_MIC_GAIN 13

/*
 * CSK6 codec gains in dB, analog (_A) and digital (_D). The TX DAC sets the
 * level into the BK4819 mic input for both FM and M17; never above 0 dB.
 */
#define C62_ADC_MIC_GAIN_A 6
#define C62_ADC_MIC_GAIN_D 10
#define C62_ADC_RX_GAIN_A 6
#define C62_ADC_RX_GAIN_D 20
#define C62_DAC_SPK_GAIN_A -6
#define C62_DAC_SPK_GAIN_D 0
#define C62_DAC_TX_GAIN_A -18
#define C62_DAC_TX_GAIN_D 0

/**
 * BK4819 and BK1080 device instances. Pin numbers must be kept in sync
 * with the board devicetree (gpioa/gpiob nodes).
 */
extern const struct BK4819 c62_bk4819;
extern const struct gpioDev c62_bk4819_gpio;
extern const struct BK1080 c62_bk1080;

extern const struct gpio_dt_spec speaker_enable;
extern const struct gpio_dt_spec dtmf_enable;
extern const struct gpio_dt_spec button_ptt;
extern const struct gpio_dt_spec led_white;
extern const struct gpio_dt_spec led_green;
extern const struct gpio_dt_spec led_keyboard;
extern const struct pwm_dt_spec pwm_lcd_backlight;
extern const struct pwm_dt_spec pwm_rf_apc;
extern const struct device *adc_dev;

#ifdef __cplusplus
}
#endif

#endif /* HWCONFIG_H */
