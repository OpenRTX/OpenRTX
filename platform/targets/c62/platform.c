/***************************************************************************
 *   Copyright (C) 2023 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
 *                         Silvano Seva IU2KWO                             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#include <interfaces/platform.h>
#include <interfaces/delays.h>
#include <hwconfig.h>

#include <zephyr/drivers/gpio.h>


#include <zephyr/logging/log.h>
#include <zephyr/drivers/uart.h>

#include <zephyr/kernel.h>

//#include <zephyr/drivers/sensor.h>
//#include <zephyr/drivers/uart.h>
//#include <zephyr/drivers/led_strip.h>
//#include <bk4819.h>

#define BUTTON_PTT_NODE DT_NODELABEL(button_ptt)

static const struct gpio_dt_spec button_ptt = GPIO_DT_SPEC_GET_OR(BUTTON_PTT_NODE, gpios, {0});
//static const struct device *const qdec_dev = DEVICE_DT_GET(DT_ALIAS(qdec0));
//static const struct device *const led_dev  = DEVICE_DT_GET(DT_ALIAS(led0));

#define SLEEP_TIME_MS   250

#define LEDWHITE_NODE DT_ALIAS(ledwhite)
static const struct gpio_dt_spec led_white = GPIO_DT_SPEC_GET(LEDWHITE_NODE, gpios);

#define LEDGREEN_NODE DT_ALIAS(ledgreen)
static const struct gpio_dt_spec led_green = GPIO_DT_SPEC_GET(LEDGREEN_NODE, gpios);

#define DISPLAY0_NODE DT_ALIAS(display0)
static const struct gpio_dt_spec disp = GPIO_DT_SPEC_GET(DISPLAY0_NODE, gpios);



// This is cross-references in keyboard_ttwrplus.c to implement volume control
// uint8_t volume_level = 125;

static hwInfo_t hwInfo =
{
    .name        = "c62",
    .hw_version  = 0,
    .uhf_band    = 1,
    .vhf_band    = 1,
    .uhf_maxFreq = 480,
    .uhf_minFreq = 400,
    .vhf_maxFreq = 174,
    .vhf_minFreq = 137,
};

// RGB led color data
//static struct led_rgb led_color = {0};

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);


void platform_init_csk6()
{

    k_msleep(SLEEP_TIME_MS);

	LOG_ERR("0x49 de OE3ANC from OPENRTX on the C62");

    printk("0x49 de OE3ANC from OPENRTX on the C62");

    int ret;

    ret = gpio_pin_configure_dt(&disp, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        return;
    }

    ret = gpio_pin_toggle_dt(&disp);
    if (ret < 0) {
        return;
    }

    k_msleep(SLEEP_TIME_MS);
   

    if (!device_is_ready(led_white.port)) {
        return;
    }


    ret = gpio_pin_configure_dt(&led_white, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        return;
    }


    ret = gpio_pin_toggle_dt(&led_white);
    if (ret < 0) {
        return;
    }

    k_msleep(SLEEP_TIME_MS);
    
    ret = gpio_pin_toggle_dt(&led_white);
    if (ret < 0) {
        return;
    }

    k_msleep(SLEEP_TIME_MS);

    if (!device_is_ready(led_white.port)) {
        return;
    }


    ret = gpio_pin_configure_dt(&led_green, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        return;
    }


    ret = gpio_pin_toggle_dt(&led_green);
    if (ret < 0) {
        return;
    }

    k_msleep(SLEEP_TIME_MS);
    
    ret = gpio_pin_toggle_dt(&led_green);
    if (ret < 0) {
        return;
    }

    ;
}

void platform_terminate()
{
    ;
}

uint16_t platform_getVbat()
{
    return 0;
}

uint8_t platform_getMicLevel()
{
    return 0;
}

uint8_t platform_getVolumeLevel()
{
    return 0;
}

int8_t platform_getChSelector()
{
    return 0;
}

bool platform_getPttStatus()
{
    return gpio_pin_get_dt(&button_ptt);
}

bool platform_pwrButtonStatus()
{

    return true;
}

void platform_ledOn(led_t led)
{
    ;
}

void platform_ledOff(led_t led)
{
    ;
}

void platform_beepStart(uint16_t freq)
{
    (void) freq;
}

void platform_beepStop()
{
    ;
}

const hwInfo_t *platform_getHwInfo()
{
    return &hwInfo;
}

