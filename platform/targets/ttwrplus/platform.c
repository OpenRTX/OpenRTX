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
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/led_strip.h>
#include <SA8x8.h>
#include <pmu.h>

#define BUTTON_PTT_NODE DT_NODELABEL(button_ptt)

static const struct gpio_dt_spec button_ptt = GPIO_DT_SPEC_GET_OR(BUTTON_PTT_NODE, gpios, {0});
static const struct device *const qdec_dev = DEVICE_DT_GET(DT_ALIAS(qdec0));
static const struct device *const led_dev  = DEVICE_DT_GET(DT_ALIAS(led0));

// This is cross-references in keyboard_ttwrplus.c to implement volume control
uint8_t volume_level = 125;

static hwInfo_t hwInfo =
{
    .name        = "ttwrplus",
    .hw_version  = 0,
    .uhf_band    = 0,
    .vhf_band    = 0,
    .uhf_maxFreq = 0,
    .uhf_minFreq = 0,
    .vhf_maxFreq = 0,
    .vhf_minFreq = 0,
    .other       = NULL,
};

// RGB led color data
static struct led_rgb led_color = {0};


void platform_init()
{
    // Setup GPIO for PTT and rotary encoder
    if(gpio_is_ready_dt(&button_ptt) == false)
        printk("Error: button device %s is not ready\n", button_ptt.port->name);

    int ret = gpio_pin_configure_dt(&button_ptt, GPIO_INPUT);
    if (ret != 0)
    {
        printk("Error %d: failed to configure %s pin %d\n", ret,
               button_ptt.port->name, button_ptt.pin);
    }

    // Rotary encoder is read using hardware pulse counter
    if(device_is_ready(qdec_dev) == false)
        printk("Qdec device is not ready\n");

    // Initialize PMU
    pmu_init();

    // Initialize LED
    if (device_is_ready(led_dev) == false)
        printk("LED device %s is not ready", led_dev->name);

    ret = led_strip_update_rgb(led_dev, &led_color, 1);
    if (ret)
        printk("couldn't update strip: %d", ret);

    // Turn on baseband and initialize the SA868 module
    pmu_setBasebandPower(true);
    sa8x8_init();

    // Detect radio model and set hwInfo accordingly
    const char *model = sa8x8_getModel();
    if(strncmp(model, "SA868S-VHF", 10) == 0)
    {
        hwInfo.vhf_band = 1;
        hwInfo.vhf_minFreq = 134;
        hwInfo.vhf_maxFreq = 174;
    }
    else if(strncmp(model, "SA868S-UHF\r", 10) == 0)
    {
        hwInfo.uhf_band = 1;
        hwInfo.uhf_minFreq = 400;
        hwInfo.uhf_maxFreq = 480;
    }
    else
    {
        printk("Error detecting SA868 model");
    }
}

void platform_terminate()
{
    pmu_terminate();
}

uint16_t platform_getVbat()
{
    return pmu_getVbat();
}

uint8_t platform_getMicLevel()
{
    return 0;
}

uint8_t platform_getVolumeLevel()
{
    return volume_level;
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
    // Long press of the power on button triggers a shutdown
    uint8_t btnStatus = pmu_pwrBtnStatus();
    if(btnStatus == 2)
        return false;

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

