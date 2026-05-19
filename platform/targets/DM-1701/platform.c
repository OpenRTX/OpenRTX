/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "interfaces/platform.h"
#include "peripherals/gpio.h"
#include "hwconfig.h"
#include <string.h>
#include "calibration/calibInfo_MDx.h"
#include "interfaces/nvmem.h"
#include "drivers/tones/toneGenerator_MDx.h"
#include "peripherals/rtc.h"
#include "interfaces/audio.h"
#include "drivers/chSelector/chSelector.h"
#include "drivers/ADC/adc_stm32.h"
#include "drivers/audio/Cx000_dac.h"

#ifdef ENABLE_BKLIGHT_DIMMING
#include "drivers/backlight/backlight.h"
#endif

static const hwInfo_t hwInfo =
{
    .vhf_maxFreq = 174,
    .vhf_minFreq = 136,
    .vhf_band    = 1,
    .uhf_maxFreq = 470,
    .uhf_minFreq = 400,
    .uhf_band    = 1,
    .hw_version  = 0,
    .name        = "DM-1701"
};


void platform_init()
{
    /* Configure GPIOs */
    gpio_setMode(GREEN_LED, OUTPUT);
    gpio_setMode(RED_LED,   OUTPUT);

    gpio_setMode(PTT_SW,  INPUT_PULL_UP);
    gpio_setMode(PTT_EXT, INPUT_PULL_UP);

    #ifndef RUNNING_TESTSUITE
    gpio_setMode(PWR_SW, OUTPUT);
    gpio_setPin(PWR_SW);
    #endif

    adcStm32_init(&adc1);

    nvm_init();                      /* Initialise non volatile memory manager */
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
    adcStm32_init(&adc1);
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
     * Battery voltage is measured through an 1:3.26 voltage divider and
     * adc_getVoltage returns a value in uV.
     */
    uint32_t vbat = adc_getVoltage(&adc1, ADC_VBAT_CH) * 326;
    return vbat / 100000;
}

uint8_t platform_getMicLevel()
{
    /* Value from ADC is 12 bit wide: shift right by four to get 0 - 255 */
    return adc_getRawSample(&adc1, ADC_MIC_CH) >> 4;
}

uint8_t platform_getVolumeLevel()
{
   /*
     * Volume level corresponds to an analog signal in the range 20 - 1650mV.
     * Potentiometer has pseudo-logarithmic law, well described with two straight
     * lines with a breakpoint around 270mV.
     * Output value has range 0 - 255 with breakpoint at 150.
     */
    uint16_t value = adc_getVoltage(&adc1, ADC_VOL_CH) / 1000;
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
    Cx000dac_startBeep(freq);
}

void platform_beepStop()
{
    Cx000dac_stopBeep();
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
 * platform/drivers/chSelector/chSelector_UV3x0.c
 */
// int8_t platform_getChSelector()
