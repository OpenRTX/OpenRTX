/***************************************************************************
 *   Copyright (C) 2020 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
 *                         Frederik Saraci IU2NRO                          *
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
 *   As a special exception, if other files instantiate templates or use   *
 *   macros or inline functions from this file, or you compile this file   *
 *   and link it with other works to produce a work based on this file,    *
 *   this file does not by itself cause the resulting work to be covered   *
 *   by the GNU General Public License. However the source code for this   *
 *   file must still be made available in accordance with the GNU General  *
 *   Public License. This exception does not invalidate any other reasons  *
 *   why a work based on this file might be covered by the GNU General     *
 *   Public License.                                                       *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#include <interfaces/platform.h>
#include <interfaces/nvmem.h>
#include <interfaces/audio.h>
#include <interfaces/gpio.h>
#include <calibInfo_GDx.h>
#include <ADC0_GDx.h>
#include <string.h>
#include <I2C0.h>
#include <pthread.h>
#include <backlight.h>
#include "hwconfig.h"

pthread_mutex_t adc_mutex;

gdxCalibration_t calibration;
hwInfo_t hwInfo;

void platform_init()
{
    /* Configure GPIOs */
    gpio_setMode(GREEN_LED, OUTPUT);
    gpio_setMode(RED_LED,   OUTPUT);

    gpio_setMode(PTT_SW, INPUT);

    gpio_setMode(PWR_SW, OUTPUT);

    backlight_init();                /* Initialise backlight driver        */
    audio_init();                    /* Initialise audio management module */
    adc0_init();                     /* Initialise ADC                     */
    pthread_mutex_init(&adc_mutex, NULL);

    /*
     * Initialise I2C driver, once for all the modules
     */
    gpio_setMode(I2C_SDA, OPEN_DRAIN);
    gpio_setMode(I2C_SCL, OPEN_DRAIN);
    gpio_setAlternateFunction(I2C_SDA, 3);
    gpio_setAlternateFunction(I2C_SCL, 3);
    i2c0_init();

    /*
     * Initialise non volatile memory manager and zero calibration data.
     * Actual loading of calibration data is deferred to the first call of
     * platform_getCalibrationData().
     */
    nvm_init();
    memset(&calibration, 0x00, sizeof(gdxCalibration_t));

    /* Initialise hardware information structure */
    hwInfo.vhf_maxFreq = 174;
    hwInfo.vhf_minFreq = 136;
    hwInfo.vhf_band    = 1;
    hwInfo.uhf_maxFreq = 470;
    hwInfo.uhf_minFreq = 400;
    hwInfo.uhf_band    = 1;
    hwInfo.lcd_type    = 0;
    memcpy(hwInfo.name, "GD-77", 5);
    hwInfo.name[5] = '\0';
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

const void *platform_getCalibrationData()
{
    /* The first time this function is called, load calibration data from flash */
    if(calibration.vhfCalPoints[0] == 0)
    {
        nvm_readCalibData(&calibration);
    }

    return ((const void *) &calibration);
}

const hwInfo_t *platform_getHwInfo()
{
    return &hwInfo;
}


/*
 * NOTE: implementation of this API function is provided in
 * platform/drivers/backlight/backlight_GDx.c
 */
// void platform_setBacklightLevel(uint8_t level)
