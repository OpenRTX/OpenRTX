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

const struct device *adc_dev = DEVICE_DT_GET(DT_NODELABEL(adc0));
