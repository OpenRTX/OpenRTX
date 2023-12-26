/***************************************************************************
 *   Copyright (C) 2020 - 2023 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN,                     *
 *                                Silvano Seva IU2KWO                      *
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
#include <peripherals/gpio.h>
#include <hwconfig.h>
#include <string.h>
#include <ADC1_MDx.h>
#include <calibInfo_MDx.h>
#include <interfaces/nvmem.h>
#include <toneGenerator_MDx.h>
#include <peripherals/rtc.h>
#include <interfaces/audio.h>
#include <chSelector.h>

#ifdef CONFIG_SCREEN_BRIGHTNESS
#include <backlight.h>
#endif

mduv3x0Calib_t calibration;
static hwInfo_t hwInfo;

void platform_init()
{
    /* Configure GPIOs */
    gpio_setMode(GREEN_LED, OUTPUT);
    gpio_setMode(RED_LED,   OUTPUT);

    gpio_setMode(PTT_SW, INPUT_PULL_UP);
    gpio_setMode(PTT_EXT, INPUT_PULL_UP);

    #ifndef RUNNING_TESTSUITE
    gpio_setMode(PWR_SW, OUTPUT);
    gpio_setPin(PWR_SW);
    #endif

    /*
     * Initialise ADC1, for vbat, RSSI, ...
     * Configuration of corresponding GPIOs in analog input mode is done inside
     * the driver.
     */
    adc1_init();

    memset(&hwInfo, 0x00, sizeof(hwInfo));

    nvm_init();                      /* Initialise non volatile memory manager */
    nvm_readCalibData(&calibration); /* Load calibration data                  */
    nvm_readHwInfo(&hwInfo);         /* Load hardware information data         */
    toneGen_init();                  /* Initialise tone generator              */
    rtc_init();                      /* Initialise RTC                         */
    chSelector_init();               /* Initialise channel selector handler    */
    audio_init();                    /* Initialise audio management module     */
}

void platform_terminate()
{
    /* Shut down LEDs */
    gpio_clearPin(GREEN_LED);
    gpio_clearPin(RED_LED);

    /* Shut down all the modules */
    adc1_terminate();
    nvm_terminate();
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
     * Volume level corresponds to an analog signal in the range 20 - 1650mV.
     * Potentiometer has pseudo-logarithmic law, well described with two straight
     * lines with a breakpoint around 270mV.
     * Output value has range 0 - 255 with breakpoint at 150.
     */
    uint16_t value = adc1_getMeasurement(ADC_VOL_CH);
    uint32_t output;

    if(value < 20)
        return 0;

    if(value <= 270)
    {
        // First line: offset zero, slope 0.556
        output = value;
        output = (output * 556) / 1000;
    }
    else
    {
        // Second line: offset 270, slope 0.076
        output  = value - 270;
        output  = (output * 76) / 1000;
        output += 150;
    }

    if(output > 255)
        output = 255;

    return output;
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
    return (platform_getVbat() > 1000) ? true : false;
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
    // calculate appropriate volume.
    uint8_t vol = platform_getVolumeLevel();
    // Since beeps have been requested, we do not want to have 0 volume.
    // We also do not want the volume to be excessive.
    if (vol < 10)
        vol = 5;
    if (vol > 176)
        vol = 176;
    toneGen_beepOn((float)freq, vol, 0);
}

void platform_beepStop()
{
    toneGen_beepOff();
}

datetime_t platform_getCurrentTime()
{
    return rtc_getTime();
}

void platform_setTime(datetime_t t)
{
    rtc_setTime(t);
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
