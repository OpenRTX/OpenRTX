/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "interfaces/platform.h"
#include "peripherals/gpio.h"
#include "interfaces/nvmem.h"
#include "interfaces/audio.h"
#include "drivers/GPIO/gpio_shiftReg.h"
#include "drivers/SPI/spi_bitbang.h"
#include "drivers/ADC/adc_stm32.h"
#include "drivers/GPS/gps_stm32.h"
#include "drivers/audio/Cx000_dac.h"
#include "hwconfig.h"
#include <string.h>
#include "core/gps.h"

static const hwInfo_t hwInfo =
{
    .name        = "CS7000P",
    .hw_version  = 0,
    .vhf_band    = 0,
    .uhf_band    = 1,
    .vhf_minFreq = 0,
    .vhf_maxFreq = 0,
    .uhf_minFreq = 400,
    .uhf_maxFreq = 527
};


void platform_init()
{
    gpio_setMode(PTT_SW,       INPUT);
    gpio_setMode(PTT_EXT,      INPUT);

    gpio_setMode(MAIN_PWR_DET, ANALOG);
    gpio_setMode(AIN_MIC,      ANALOG);
    gpio_setMode(AIN_VOLUME,   ANALOG);

    gpio_setMode(GPIOEXT_CLK, OUTPUT);
    gpio_setMode(GPIOEXT_DAT, OUTPUT);

    spi_init((const struct spiDevice *) &spiSr);
    gpioShiftReg_init(&extGpio);
    adcStm32_init(&adc1);
    nvm_init();
    audio_init();

    #ifndef RUNNING_TESTSUITE
    gpioDev_set(MAIN_PWR_SW);
    #endif
}

void platform_terminate()
{
    adcStm32_terminate(&adc1);
    gpsStm32_terminate();

    #ifndef RUNNING_TESTSUITE
    gpioDev_clear(MAIN_PWR_SW);
    #endif

    gpioShiftReg_terminate(&extGpio);
}

uint16_t platform_getVbat()
{
    /*
     * Battery voltage is measured through an 1:3.95 voltage divider and
     * adc1_getMeasurement returns a value in uV.
     */
    uint32_t vbat = adc_getVoltage(&adc1, ADC_VBAT_CH) * 395;
    return vbat / 100000;
}

uint8_t platform_getMicLevel()
{
    // ADC1 returns a 16-bit value: shift right by eight to get 0 - 255
    return adc_getRawSample(&adc1, ADC_MIC_CH) >> 8;
}

uint8_t platform_getVolumeLevel()
{
    /*
     * Volume level corresponds to an analog signal in the range 20 - 2520mV.
     * Potentiometer has pseudo-logarithmic law, well described with two straight
     * lines with a breakpoint around 410mV.
     * Output value has range 0 - 255 with breakpoint at 139.
     */
    uint16_t value = adc_getRawSample(&adc1, ADC_VOL_CH) >> 4;
    uint32_t output;

    if(value <= 512)
    {
        // First line: offset zero, slope 0.271
        output = value;
        output = (output * 271) / 1000;
    }
    else
    {
        // Second line: offset 512, slope 0.044
        output  = value - 512;
        output  = (output * 44) / 1000;
        output += 139;
    }

    if(output > 255)
        output = 255;

    return output;
}

// int8_t platform_getChSelector()
// {
//     return 0;    // TODO
// }

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
    uint16_t vbat = platform_getVbat();
    if(vbat < 1000)
        return false;

    return true;
}

void platform_ledOn(led_t led)
{
    switch(led)
    {
        case GREEN:
            gpioDev_set(GREEN_LED);
            break;

        case RED:
            gpioDev_set(RED_LED);
            break;

        case YELLOW:
            gpioDev_set(GREEN_LED);
            gpioDev_set(RED_LED);
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
            gpioDev_clear(GREEN_LED);
            break;

        case RED:
            gpioDev_clear(RED_LED);
            break;

        case YELLOW:
            gpioDev_clear(GREEN_LED);
            gpioDev_clear(RED_LED);
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

const hwInfo_t *platform_getHwInfo()
{
    return &hwInfo;
}

const struct gpsDevice *platform_initGps()
{
    gpio_setMode(GPS_RXD, ALTERNATE | ALTERNATE_FUNC(7));
    gpsStm32_init(9600);

    return &gps;
}
