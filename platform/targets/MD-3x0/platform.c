/***************************************************************************
 *   Copyright (C) 2020 by Federico Amedeo Izzo IU2NUO,                    *
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

#include <interfaces/gpio.h>
#include <interfaces/nvmem.h>
#include <interfaces/platform.h>
#include <hwconfig.h>
#include <string.h>
#include <ADC1_MDx.h>
#include <backlight.h>
#include <calibInfo_MDx.h>
#include <toneGenerator_MDx.h>
#include <interfaces/rtc.h>
#include <interfaces/audio.h>
#include <interfaces/usb.h>

md3x0Calib_t calibration;
hwInfo_t hwInfo;

void platform_init()
{
    /* Configure GPIOs */
    gpio_setMode(GREEN_LED, OUTPUT);
    gpio_setMode(RED_LED, OUTPUT);

    gpio_setMode(LCD_BKLIGHT, ALTERNATE);
    gpio_setAlternateFunction(LCD_BKLIGHT, 3);

    gpio_setMode(CH_SELECTOR_0, INPUT);
    gpio_setMode(CH_SELECTOR_1, INPUT);
    gpio_setMode(CH_SELECTOR_2, INPUT);
    gpio_setMode(CH_SELECTOR_3, INPUT);

    gpio_setMode(PTT_SW, INPUT);

    gpio_setMode(PWR_SW, OUTPUT);

    /*
     * Initialise ADC1, for vbat, RSSI, ...
     * Configuration of corresponding GPIOs in analog input mode is done inside
     * the driver.
     */
    adc1_init();

    memset(&hwInfo, 0x00, sizeof(hwInfo));

    nvm_init();                      /* Initialise non volatile memory manager */
    nvm_readCalibData(&calibration); /* Load calibration data                  */
    nvm_loadHwInfo(&hwInfo);         /* Load hardware information data         */
    toneGen_init();                  /* Initialise tone generator              */
    rtc_init();                      /* Initialise RTC                         */
    backlight_init();                /* Initialise backlight driver            */
    audio_init();                    /* Initialise audio management module     */
    usb_init();                      /* Initialise USB driver                  */
}

void platform_terminate()
{
    /* Shut down backlight */
    backlight_terminate();

    /* Shut down LEDs */
    gpio_clearPin(GREEN_LED);
    gpio_clearPin(RED_LED);

    /* Shut down all the modules */
    adc1_terminate();
    nvm_terminate();
    toneGen_terminate();
    rtc_terminate();
    audio_terminate();

    /* Finally, remove power supply */
    gpio_clearPin(PWR_SW);
}

float platform_getVbat()
{
    /*
     * Battery voltage is measured through an 1:3 voltage divider and
     * adc1_getMeasurement returns a value in mV. Thus, to have effective
     * battery voltage multiply by three and divide by 1000
     */
    return adc1_getMeasurement(ADC_VBAT_CH)*3.0f/1000.0f;
}

float platform_getMicLevel()
{
    return adc1_getMeasurement(ADC_VOX_CH);
}

float platform_getVolumeLevel()
{
    return adc1_getMeasurement(ADC_VOL_CH);
}

int8_t platform_getChSelector()
{
    static const uint8_t rsPositions[] = { 11, 14, 10, 15, 6, 3, 7, 2, 12, 13,
                                           9, 16, 5, 4, 8, 1 };
    int pos = gpio_readPin(CH_SELECTOR_0)
            | (gpio_readPin(CH_SELECTOR_1) << 1)
            | (gpio_readPin(CH_SELECTOR_2) << 2)
            | (gpio_readPin(CH_SELECTOR_3) << 3);
    return rsPositions[pos];
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
    return ((const void *) &calibration);
}

const hwInfo_t *platform_getHwInfo()
{
    return &hwInfo;
}

/*
 * NOTE: implementation of this API function is provided in
 * platform/drivers/backlight/backlight_MDx.c
 */
// void platform_setBacklightLevel(uint8_t level)
