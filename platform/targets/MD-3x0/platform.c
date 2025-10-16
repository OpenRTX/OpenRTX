/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "peripherals/gpio.h"
#include "interfaces/nvmem.h"
#include "interfaces/platform.h"
#include "interfaces/delays.h"
#include "hwconfig.h"
#include <string.h>
#include "drivers/ADC/adc_stm32.h"
#include "calibration/calibInfo_MDx.h"
#include "drivers/tones/toneGenerator_MDx.h"
#include "peripherals/rtc.h"
#include "interfaces/audio.h"
#include "drivers/GPS/gps_stm32.h"
#include "core/gps.h"

static hwInfo_t hwInfo;

void platform_init()
{
    /* Configure GPIOs */
    gpio_setMode(GREEN_LED, OUTPUT);
    gpio_setMode(RED_LED, OUTPUT);

    gpio_setMode(CH_SELECTOR_0, INPUT);
    gpio_setMode(CH_SELECTOR_1, INPUT);
    gpio_setMode(CH_SELECTOR_2, INPUT);
    gpio_setMode(CH_SELECTOR_3, INPUT);

    gpio_setMode(PTT_SW,  INPUT_PULL_UP);
    gpio_setMode(PTT_EXT, INPUT_PULL_UP);

    gpio_setMode(AIN_VBAT,   ANALOG);
    gpio_setMode(AIN_VOLUME, ANALOG);
    gpio_setMode(AIN_MIC,    ANALOG);
    gpio_setMode(AIN_RSSI,   ANALOG);

    #ifndef RUNNING_TESTSUITE
    gpio_setMode(PWR_SW, OUTPUT);
    gpio_setPin(PWR_SW);
    #endif

    /* Initialise ADC1, for vbat, RSSI, ... */
    adcStm32_init(&adc1);

    memset(&hwInfo, 0x00, sizeof(hwInfo));

    nvm_init();                      /* Initialise non volatile memory manager */
    nvm_readHwInfo(&hwInfo);         /* Load hardware information data         */
    toneGen_init();                  /* Initialise tone generator              */
    rtc_init();                      /* Initialise RTC                         */
    audio_init();                    /* Initialise audio management module     */
}

void platform_terminate()
{
    /* Shut down LEDs */
    gpio_clearPin(GREEN_LED);
    gpio_clearPin(RED_LED);

    /* Shut down all the modules */
    adcStm32_terminate(&adc1);
    gpsStm32_terminate();
    nvm_terminate();
    toneGen_terminate();
    audio_terminate();

    /* Finally, remove power supply */
    gpio_clearPin(PWR_SW);
}

uint16_t platform_getVbat()
{
    /*
     * Battery voltage is measured through an 1:3 voltage divider and
     * adc1_getMeasurement returns a value in uV.
     */
    uint32_t vbat = adc_getVoltage(&adc1, ADC_VBAT_CH) * 3;
    return vbat / 1000;
}

uint8_t platform_getMicLevel()
{
    /* Value from ADC is 12 bit wide: shift right by four to get 0 - 255 */
    return adc_getRawSample(&adc1, ADC_VOX_CH) >> 4;
}

uint8_t platform_getVolumeLevel()
{
    /*
     * Volume level corresponds to an analog signal in the range 0 - 1650mV.
     * Potentiometer has pseudo-logarithmic law, well described with two straight
     * lines with a breakpoint around 270mV.
     * Output value has range 0 - 255 with breakpoint at 150.
     */
    uint32_t value = adc_getVoltage(&adc1, ADC_VOL_CH) / 1000;
    uint32_t output;

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

const struct gpsDevice *platform_initGps()
{
    const struct gpsDevice *dev = NULL;

    // Turn on the GPS and check if there is voltage on the RXD pin
    gpio_setMode(GPS_DATA, INPUT_PULL_DOWN);
    gpio_setMode(GPS_EN, OUTPUT);
    gpio_setPin(GPS_EN);

    for(size_t i = 0; i < 50; i++) {
        if(gpio_readPin(GPS_DATA) != 0) {
            dev = &gps;
            gpsStm32_init(9600);
            break;
        }

        sleepFor(0, 1);
    }

    gpio_clearPin(GPS_EN);
    gpio_setMode(GPS_DATA, ALTERNATE | ALTERNATE_FUNC(7));

    return dev;
}
