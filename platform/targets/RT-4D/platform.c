/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <drivers/ADC/adc_at32.h>
#include "drivers/SPI/spi_bitbang.h"
#include <hwconfig.h>
#include <interfaces/nvmem.h>
#include <interfaces/platform.h>
#include <peripherals/gpio.h>

static hwInfo_t hwInfo = {
    .vhf_maxFreq = 174,
    .vhf_minFreq = 136,
    .vhf_band = 1,
    .uhf_maxFreq = 480,
    .uhf_minFreq = 400,
    .uhf_band = 1,
    .hw_version = 0,
    .flags = 0,
    .name = "RT-4D",
};

void platform_init()
{
    // Configure GPIOs
    gpio_setMode(GREEN_LED, OUTPUT);
    gpio_setMode(RED_LED, OUTPUT);
    gpio_setMode(PTT_SW, INPUT);
    gpio_setMode(AIN_VBAT, ANALOG);

    adcAt32_init(&adc1);
    nvm_init();
}

void platform_terminate()
{
    /* Shut down LEDs */
    gpio_clearPin(GREEN_LED);
    gpio_clearPin(RED_LED);
}

uint16_t platform_getVbat()
{
    /*
     * Battery voltage is measured through an 1:7.4 voltage divider and
     * adc1_getMeasurement returns a value in uV.
     */
    uint32_t vbat = adc_getVoltage(&adc1, ADC_VBAT_CH) * 74;
    return vbat / 10000;
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
    /* PTT line has a pullup resistor with PTT switch closing to ground */
    return (gpio_readPin(PTT_SW) == 0) ? true : false;
}

bool platform_pwrButtonStatus()
{
    return true;
}

void platform_ledOn(led_t led)
{
    switch (led) {
        case RED:
            gpio_setPin(RED_LED);
            break;

        case GREEN:
            gpio_setPin(GREEN_LED);
            break;

        default:
            break;
    }
}

void platform_ledOff(led_t led)
{
    switch (led) {
        case RED:
            gpio_clearPin(RED_LED);
            break;

        case GREEN:
            gpio_clearPin(GREEN_LED);
            break;

        default:
            break;
    }
}

void platform_beepStart(uint16_t freq)
{
    /* TODO */
    (void)freq;
}

void platform_beepStop()
{
    /* TODO */
}

const hwInfo_t *platform_getHwInfo()
{
    return &hwInfo;
}
