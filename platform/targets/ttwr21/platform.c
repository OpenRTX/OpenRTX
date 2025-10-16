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

#include "interfaces/platform.h"
#include "interfaces/delays.h"
#include "hwconfig.h"
#include "core/gps.h"
#include "sa8x8-fw-sa868s-uhf.h"

#include "drivers/GPS/gps_zephyr.h"
#include "drivers/baseband/SA8x8.h"
#include "drivers/program/program_RL78.h"
#include "drivers/PMU/AXP2101_zephyr.h"

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/uart.h>

enum ttwr21_band {
  VHF = 0,
  MHZ350 = 1,
  UHF = 2,
};

#define BUTTON_PTT_NODE DT_NODELABEL(button_ptt)
#define BUTTON_MODE_NODE DT_NODELABEL(button_mode)

static const struct gpio_dt_spec button_ptt = GPIO_DT_SPEC_GET_OR(BUTTON_PTT_NODE, gpios, {0});
static const struct gpio_dt_spec button_mode = GPIO_DT_SPEC_GET_OR(BUTTON_MODE_NODE, gpios, {0});

static const struct device *const qdec_dev = DEVICE_DT_GET(DT_ALIAS(qdec0));
static const struct device *const led_dev  = DEVICE_DT_GET(DT_ALIAS(led0));

// This is cross-references in keyboard_ttwrplus.c to implement volume control
uint8_t volume_level = 125;

static hwInfo_t hwInfo =
{
    .name        = "ttwr21",
    .hw_version  = 0,
    .uhf_band    = 0,
    .vhf_band    = 0,
    .uhf_maxFreq = 0,
    .uhf_minFreq = 0,
    .vhf_maxFreq = 0,
    .vhf_minFreq = 0,
};

// RGB led color data
static struct led_rgb led_color = {0};

static void gpsEnable(void *priv)
{
    (void) priv;
    pmu_setGPSPower(true);
}

static void gpsDisable(void *priv)
{
    (void) priv;
    pmu_setGPSPower(false);
}

static const struct gpsDevice gps =
{
    .enable = gpsEnable,
    .disable = gpsDisable,
    .getSentence = gpsZephyr_getNmeaSentence
};

#define I2C_DEV_NODE    DT_ALIAS(i2c_0)

// Try reading i2c register to detect radio band
// VHF = 0x3c
// UHF = 0x3d
// 300MHz = ???
int8_t detect_band()
{
    static const struct device *const i2c_dev = DEVICE_DT_GET(I2C_DEV_NODE);
    if (!i2c_dev) {
        printk("I2C device not found!\n");
        return -1;
    }

    uint8_t val;
    if(!i2c_reg_read_byte(i2c_dev, 0x3c, 0x00, &val)) {
        return VHF;
    } else if(!i2c_reg_read_byte(i2c_dev, 0x3d, 0x00, &val)) {
        return UHF;
    } else {
        printk("Error detecting supported band!\n");
    }
}

void platform_init()
{
    int ret;

    // Setup GPIO for PTT and rotary encoder
    if (!gpio_is_ready_dt(&button_ptt))
        printk("Error: button device %s is not ready\n", button_ptt.port->name);

    if ((ret = gpio_pin_configure_dt(&button_ptt, GPIO_INPUT)))
        printk("Error %d: failed to configure %s pin %d\n", ret,
               button_ptt.port->name, button_ptt.pin);

    if (!gpio_is_ready_dt(&button_mode))
        printk("Error: button device %s is not ready\n", button_mode.port->name);

    if ((ret = gpio_pin_configure_dt(&button_mode, GPIO_INPUT)))
        printk("Error %d: failed to configure %s pin %d\n", ret,
               button_mode.port->name, button_mode.pin);

    // Rotary encoder is read using hardware pulse counter
    if (!device_is_ready(qdec_dev))
        printk("Qdec device is not ready\n");

    // Initialize LED
    if (!device_is_ready(led_dev))
        printk("LED device %s is not ready\n", led_dev->name);

    // Update LED
    if ((ret = led_strip_update_rgb(led_dev, &led_color, 1)))
        printk("LED not updated: %d\n", ret);

    // Initialize PMU
    pmu_init();

    if (gpio_pin_get_dt(&button_mode)) {
        led_color.r = 0x50;
        led_color.g = 0x50;
        led_color.b = 0x00;
        led_strip_update_rgb(led_dev, &led_color, 1);

        pmu_setBasebandProgramPower(true);

        // Flash a supported baseband firmware
        if (rl78_flash(sa8x8_fw, sa8x8_fw_len)) {
            led_color.r = 0x50;
            led_color.g = 0x00;
            led_color.b = 0x00;
            printk("Failed to flash baseband\n");
        } else {
            led_color.r = 0x00;
            led_color.g = 0x50;
            led_color.b = 0x00;
        }

        led_strip_update_rgb(led_dev, &led_color, 1);

        delayMs(3000);

        led_color.r = 0x00;
        led_color.g = 0x00;
        led_color.b = 0x00;

        led_strip_update_rgb(led_dev, &led_color, 1);

        pmu_setBasebandProgramPower(false);
    }

    // Detect radio model and set hwInfo accordingly
    enum ttwr21_band band = detect_band();
    switch(band) {
        case VHF:
            hwInfo.vhf_band = 1;
            hwInfo.vhf_minFreq = 134;
            hwInfo.vhf_maxFreq = 174;
            break;
        case UHF:
            hwInfo.uhf_band = 1;
            hwInfo.uhf_minFreq = 400;
            hwInfo.uhf_maxFreq = 480;
            break;
        case MHZ350:
            printk("Error: 350MHz band not supported!\n");
            break;
    }

    // Enable power to baseband
    pmu_setBasebandPower(true);

    // Initialize baseband
    if (sa8x8_init() == -2) {
        // if (sa8x8_init()) {
        //     printk("Error detecting baseband after firmware update\n");
        // }
    }

    // Confirm that SA8x8 has the fw of the corresponding band
    const char *model = sa8x8_getModel();
    if(!strncmp(model, "SA868S-VHF", 10)) {
        if(!hwInfo.vhf_band) {
            printk("Mismatch between radio band and baseband firmware!");
        }
    } else if(!strncmp(model, "SA868S-UHF", 10)) {
        if(!hwInfo.uhf_band) {
            printk("Mismatch between radio band and baseband firmware!");
        }
    } else {
        printk("Error detecting baseband model\n");
    }

    // Connect SA868s to PA
    pmu_setAudioSwitch(false);
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
        printk("LED not updated: %d\n", ret);
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
        printk("LED not updated: %d\n", ret);
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

const struct gpsDevice *platform_initGps()
{
    gpsZephyr_init();

    return &gps;
}
