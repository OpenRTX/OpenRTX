/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <zephyr/drivers/display.h>
#include <zephyr/drivers/pwm.h>
#include <interfaces/display.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <hwconfig.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(display, LOG_LEVEL_INF);

/*
 * The ST7735R takes RGB565 pixels, 16 bits each, with the high byte first:
 * the framebuffer is already in that order, see CONFIG_PIX_FMT_RGB565_BE.
 */
#define BYTES_PER_PIXEL 2
#define FB_SIZE (CONFIG_SCREEN_HEIGHT * CONFIG_SCREEN_WIDTH * BYTES_PER_PIXEL)

/* Backlight PWM period: 1 kHz */
#define BACKLIGHT_PERIOD_US 1000
/*
 * Backlight curve, quadratic for a finer control of the low levels: duty in
 * percent = level^2 / BACKLIGHT_CURVE_DIV + BACKLIGHT_MIN_DUTY, which is 100
 * at level 100.
 */
#define BACKLIGHT_CURVE_DIV 105
#define BACKLIGHT_MIN_DUTY 5

static const struct device *displayDev;
static struct display_buffer_descriptor displayBufDesc = {
    .buf_size = FB_SIZE,
    .width = CONFIG_SCREEN_WIDTH,
    .height = CONFIG_SCREEN_HEIGHT,
    .pitch = CONFIG_SCREEN_WIDTH,
};

void display_init()
{
    LOG_INF("Display init");
    displayDev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

    if (!device_is_ready(displayDev)) {
        LOG_ERR("Display device not ready");
        return;
    }

    // Turn display on
    display_blanking_off(displayDev);
    LOG_INF("Display init done");
}

void display_terminate()
{
    if (displayDev != NULL) {
        display_blanking_on(displayDev);
    }
}

void display_renderRows(uint8_t startRow, uint8_t endRow, void *fb)
{
    if ((startRow >= CONFIG_SCREEN_HEIGHT) || (endRow >= CONFIG_SCREEN_HEIGHT)
        || (startRow > endRow)) {
        return;
    }

    const size_t rowSize = CONFIG_SCREEN_WIDTH * BYTES_PER_PIXEL;
    const uint16_t rowCount = endRow - startRow + 1;
    const struct display_buffer_descriptor bufDesc = {
        .buf_size = rowSize * rowCount,
        .width = CONFIG_SCREEN_WIDTH,
        .height = rowCount,
        .pitch = CONFIG_SCREEN_WIDTH,
    };

    display_write(displayDev, 0, startRow, &bufDesc,
                  (const uint8_t *)fb + (startRow * rowSize));
}

void display_render(void *fb)
{
    display_write(displayDev, 0, 0, &displayBufDesc, fb);
}

void display_setBacklightLevel(uint8_t level)
{
    if (level > 100)
        level = 100;

    uint32_t duty = 0;

    // Rounded to the nearest percent
    if (level > 0) {
        duty = ((level * level) + (BACKLIGHT_CURVE_DIV / 2))
             / BACKLIGHT_CURVE_DIV;
        duty += BACKLIGHT_MIN_DUTY;
    }

    uint32_t pulse_us = (BACKLIGHT_PERIOD_US * duty) / 100;

    int ret = pwm_set_dt(&pwm_lcd_backlight, PWM_USEC(BACKLIGHT_PERIOD_US),
                         PWM_USEC(pulse_us));

    if (ret < 0) {
        LOG_ERR("Failed to set display PWM: %d", ret);
    }

    // Keyboard backlight does not have dimming, only on/off
    if (level > 0)
        gpio_pin_set_dt(&led_keyboard, 1);
    else
        gpio_pin_set_dt(&led_keyboard, 0);
}
