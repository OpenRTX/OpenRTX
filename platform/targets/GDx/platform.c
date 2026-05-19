/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "interfaces/platform.h"
#include "interfaces/nvmem.h"
#include "interfaces/audio.h"
#include "peripherals/gpio.h"
#include "calibration/calibInfo_GDx.h"
#include "drivers/ADC/ADC0_GDx.h"
#include <string.h>
#include "I2C0.h"
#include <pthread.h>
#include "drivers/backlight/backlight.h"
#include "hwconfig.h"

pthread_mutex_t adc_mutex;

gdxCalibration_t calibration;

static const hwInfo_t hwInfo =
{
    .vhf_maxFreq = 174,
    .vhf_minFreq = 136,
    .vhf_band    = 1,
    .uhf_maxFreq = 470,
    .uhf_minFreq = 400,
    .uhf_band    = 1,
    .hw_version  = 0,
    #ifdef PLATFORM_GD77
    .name        = "GD-77"
    #else
    .name        = "DM-1801"
    #endif
};

void platform_init()
{
    /* Configure GPIOs */
    gpio_setMode(GREEN_LED, OUTPUT);
    gpio_setMode(RED_LED,   OUTPUT);

    gpio_setMode(PTT_SW, INPUT);

    #ifndef RUNNING_TESTSUITE
    gpio_setMode(PWR_SW, OUTPUT);
    #endif

    backlight_init();                /* Initialise backlight driver        */
    audio_init();                    /* Initialise audio management module */
    adc0_init();                     /* Initialise ADC                     */
    nvm_init();                      /* Initialise NVM manager             */
    pthread_mutex_init(&adc_mutex, NULL);

    /*
     * Initialise I2C driver, once for all the modules
     */
    gpio_setMode(I2C_SDA, OPEN_DRAIN | ALTERNATE_FUNC(5));
    gpio_setMode(I2C_SCL, OPEN_DRAIN | ALTERNATE_FUNC(5));
    i2c0_init();
}

void platform_terminate()
{
    /* Shut down backlight */
    backlight_terminate();

    gpio_clearPin(RED_LED);
    gpio_clearPin(GREEN_LED);

    adc0_terminate();
    pthread_mutex_destroy(&adc_mutex);

    i2c0_terminate();
    audio_terminate();

    /* Finally, remove power supply */
    gpio_clearPin(PWR_SW);
}

uint16_t platform_getVbat()
{
    pthread_mutex_lock(&adc_mutex);
    uint16_t value = adc0_getMeasurement(1);
    pthread_mutex_unlock(&adc_mutex);

    /*
     * Battery voltage is measured through an 1:3 voltage divider and
     * adc1_getMeasurement returns a value in mV. Thus, to have effective
     * battery voltage, multiply by three.
     */
    return value * 3;
}

uint8_t platform_getMicLevel()
{
    pthread_mutex_lock(&adc_mutex);
    uint16_t value = adc0_getRawSample(3);
    pthread_mutex_unlock(&adc_mutex);

    /* Value from ADC is 12 bit wide: shift right by four to get 0 - 255 */
    return value >> 4;
}

uint8_t platform_getVolumeLevel()
{
    /* TODO */
    return 0;
}

int8_t platform_getChSelector()
{
    /* GD77 does not have a channel selector */
    return 0;
}

bool platform_getPttStatus()
{
    /* PTT line has a pullup resistor with PTT switch closing to ground */
    return (gpio_readPin(PTT_SW) == 0) ? true : false;
}

bool platform_pwrButtonStatus()
{
    /*
     * When power knob is set to off, battery voltage measurement returns 0V.
     * Here we set the threshold to 1V since, with knob in off position, there
     * is always a bit of noise in the ADC measurement making the returned
     * voltage not to be exactly zero.
     */
    return (platform_getVbat() > 1.0f) ? true : false;
}

void platform_ledOn(led_t led)
{
    switch(led)
    {
        case GREEN:
            gpio_setPin(GREEN_LED);
            break;

        case RED:
            gpio_setPin(RED_LED);
            break;

        default:
            break;
    }
}

void platform_ledOff(led_t led)
{
    switch(led)
    {
        case GREEN:
            gpio_clearPin(GREEN_LED);
            break;

        case RED:
            gpio_clearPin(RED_LED);
            break;

        default:
            break;
    }
}

void platform_beepStart(uint16_t freq)
{
    /* TODO */

    (void) freq;
}

void platform_beepStop()
{
    /* TODO */
}

const hwInfo_t *platform_getHwInfo()
{
    return &hwInfo;
}
