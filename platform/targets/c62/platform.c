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

//#include <zephyr/drivers/gpio.h>
//#include <zephyr/drivers/sensor.h>
//#include <zephyr/drivers/uart.h>
//#include <zephyr/drivers/led_strip.h>
//#include <bk4819.h>

#define BUTTON_PTT_NODE DT_NODELABEL(button_ptt)

static const struct gpio_dt_spec button_ptt = GPIO_DT_SPEC_GET_OR(BUTTON_PTT_NODE, gpios, {0});
static const struct device *const qdec_dev = DEVICE_DT_GET(DT_ALIAS(qdec0));
static const struct device *const led_dev  = DEVICE_DT_GET(DT_ALIAS(led0));

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
static struct led_rgb led_color = {0};


void platform_init()
{
    return 0;
}

void platform_terminate()
{
    return 0;
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
    int rc = sensor_sample_fetch(qdec_dev);
    if (rc != 0)
    {
        printk("Failed to fetch sample (%d)\n", rc);
        return 0;
    }

    struct sensor_value val;
    rc = sensor_channel_get(qdec_dev, SENSOR_CHAN_ROTATION, &val);
    if (rc != 0)
    {
        printk("Failed to get data (%d)\n", rc);
        return 0;
    }

    // The esp32-pcnt sensor returns a signed 16-bit value: we remap it into a
    // signed 8-bit one.
    return (int8_t) val.val1;
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
    int ret = 0;

    switch(led)
    {
        case GREEN:
            led_color.g = 0xff;
            break;
        case RED:
            led_color.r = 0xff;
            break;
        default:
            break;
    }

    ret = led_strip_update_rgb(led_dev, &led_color, 1);
    if (ret)
        printk("couldn't update strip: %d", ret);
}

void platform_ledOff(led_t led)
{
    int ret = 0;

    switch(led)
    {
        case GREEN:
            led_color.g = 0x00;
            break;
        case RED:
            led_color.r = 0x00;
            break;
        default:
            break;
    }

    ret = led_strip_update_rgb(led_dev, &led_color, 1);
    if (ret)
        printk("couldn't update strip: %d", ret);
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

