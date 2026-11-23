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
#include "pinmap.h"

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

/* ADC channels */
enum AdcChannels {
    ADC_CHARGE_CH = 1, ///< Charge detection.
    ADC_VBAT_CH = 2,   ///< Battery voltage, pin B08.
};

#define CONFIG_M17

/* Software gain of the M17 microphone input, set for C62_ADC_MIC_GAIN_A/D */
#define CONFIG_MIC_GAIN 13

/* GPIO controllers, for the pins driven through the gpioPin API */
extern const struct gpioDev GpioA;
extern const struct gpioDev GpioB;

/* BK4819 and BK1080 device instances */
extern const struct BK4819 c62_bk4819;
extern const struct gpioDev c62_bk4819_gpio;

extern const struct BK1080 c62_bk1080;

/* CSK6 codec gains: mic and speaker on the left, BK4819 on the right */
extern const struct csk6codecCfg c62_codecCfg;

extern const struct gpio_dt_spec speaker_enable;
extern const struct gpio_dt_spec dtmf_enable;
extern const struct gpio_dt_spec button_ptt;
extern const struct gpio_dt_spec led_white;
extern const struct gpio_dt_spec led_green;
extern const struct gpio_dt_spec led_keyboard;
extern const struct pwm_dt_spec pwm_lcd_backlight;
extern const struct pwm_dt_spec pwm_rf_apc;
extern const struct device *adc_dev;

/*
 * Persistence policy.
 *
 * The C62 has no graceful shutdown: the power knob cuts power, so
 * state_terminate() never runs and a save on shutdown would be lost. Settings
 * and VFO changes are therefore persisted by a debounced write while the
 * radio is running. Platforms with a clean shutdown leave this undefined and
 * keep saving from state_terminate() only.
 */
#define NVMEM_DEBOUNCED_WRITE
#define NVM_WRITE_DEBOUNCE_MS 3000

#ifdef __cplusplus
}
#endif

#endif /* HWCONFIG_H */
