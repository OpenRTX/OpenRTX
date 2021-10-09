/***************************************************************************
 *   Copyright (C) 2020 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN,                            *
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
#include <interfaces/gpio.h>
#include <hwconfig.h>
#include <string.h>
#include <ADC1_MDx.h>
#include <calibInfo_MDx.h>
#include <interfaces/nvmem.h>
#include <toneGenerator_MDx.h>
#include <interfaces/rtc.h>
#include <interfaces/audio.h>
#include <chSelector.h>

#ifdef ENABLE_BKLIGHT_DIMMING
#include <backlight.h>
#endif

mduv3x0Calib_t calibration;
hwInfo_t hwInfo;

void platform_init()
{
    /* Configure GPIOs */
    gpio_setMode(GREEN_LED, OUTPUT);
    gpio_setMode(RED_LED,   OUTPUT);

    gpio_setMode(PTT_SW, INPUT_PULL_UP);
    gpio_setMode(PTT_EXT, INPUT_PULL_UP);

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
    chSelector_init();               /* Initialise channel selector handler    */
    audio_init();                    /* Initialise audio management module     */

    #ifdef ENABLE_BKLIGHT_DIMMING
    backlight_init();                /* Initialise backlight driver            */
    #else
    gpio_setMode(LCD_BKLIGHT, OUTPUT);
    gpio_clearPin(LCD_BKLIGHT);
    #endif
}

void platform_terminate()
{
    /* Shut down backlight */
    #ifdef ENABLE_BKLIGHT_DIMMING
    backlight_terminate();
    #else
    gpio_clearPin(LCD_BKLIGHT);
    #endif

    /* Shut down LEDs */
    gpio_clearPin(GREEN_LED);
    gpio_clearPin(RED_LED);

    /* Shut down all the modules */
    adc1_terminate();
    nvm_terminate();
    rtc_terminate();
    toneGen_terminate();
    chSelector_terminate();
    audio_terminate();

    /* Finally, remove power supply */
    gpio_clearPin(PWR_SW);
}

uint16_t platform_getVbat()
{
    /*
     * Battery voltage is measured through an 1:3 voltage divider and
     * adc1_getMeasurement returns a value in mV. Thus, to have effective
     * battery voltage, multiply by three.
     */
    return adc1_getMeasurement(ADC_VBAT_CH) * 3;
}

uint8_t platform_getMicLevel()
{
    /* Value from ADC is 12 bit wide: shift right by four to get 0 - 255 */
    return (adc1_getRawSample(ADC_VOX_CH) >> 4);
}

uint8_t platform_getVolumeLevel()
{
    /*
     * Knob position corresponds to an analog signal in the range 0 - 1600mV,
     * converted to a value in range 0 - 255 using fixed point math: divide by
     * 1600 and then multiply by 256.
     */
    uint16_t value = adc1_getMeasurement(ADC_VOL_CH);
    if(value > 1599) value = 1599;
    uint32_t level = value << 16;
    level /= 1600;
    return ((uint8_t) (level >> 8));
}

bool platform_getPttStatus()
{
    /* PTT line has a pullup resistor with PTT switch closing to ground */
    uint8_t intPttStatus = gpio_readPin(PTT_SW);
    uint8_t extPttStatus = gpio_readPin(PTT_EXT);
    return ((intPttStatus == 0) || (extPttStatus == 0)) ? true : false;
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
 * platform/drivers/chSelector/chSelector_MDUV3x0.c
 */
// int8_t platform_getChSelector()

/*
 * NOTE: when backligth dimming is enabled, the implementation of this API
 * function is provided in platform/drivers/backlight/backlight_MDx.c to avoid
 * an useless function call.
 */
#ifndef ENABLE_BKLIGHT_DIMMING
void platform_setBacklightLevel(uint8_t level)
{
    if(level > 1)
    {
        gpio_setPin(LCD_BKLIGHT);
    }
    else
    {
        gpio_clearPin(LCD_BKLIGHT);
    }
}
#endif
