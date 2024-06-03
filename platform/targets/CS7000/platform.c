/***************************************************************************
 *   Copyright (C) 2024 by Silvano Seva IU2KWO                             *
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
#include <interfaces/nvmem.h>
#include <gpio_shiftReg.h>
#include <spi_bitbang.h>
#include <adc_stm32.h>
#include <hwconfig.h>
#include <string.h>

static const hwInfo_t hwInfo =
{
    .name        = "CS7000",
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

    #ifndef RUNNING_TESTSUITE
    gpioDev_set(MAIN_PWR_SW);
    #endif
}

void platform_terminate()
{
    adcStm32_terminate(&adc1);

    #ifndef RUNNING_TESTSUITE
    gpioDev_clear(MAIN_PWR_SW);
    #endif

    gpioShiftReg_terminate(&extGpio);
}

uint16_t platform_getVbat()
{
    /*
     * Battery voltage is measured through an 1:3.9 voltage divider and
     * adc1_getMeasurement returns a value in uV.
     */
    uint32_t vbat = adc_getVoltage(&adc1, ADC_VBAT_CH) * 39;
    return vbat / 10000;
}

uint8_t platform_getMicLevel()
{
    /* Value from ADC is 12 bit wide: shift right by four to get 0 - 255 */
    return adc_getRawSample(&adc1, ADC_MIC_CH) >> 4;
}

uint8_t platform_getVolumeLevel()
{
    /*
     * Volume level corresponds to an analog signal in the range 20 - 2520mV.
     * Potentiometer has pseudo-logarithmic law, well described with two straight
     * lines with a breakpoint around 410mV.
     * Output value has range 0 - 255 with breakpoint at 139.
     */
    uint16_t value = adc_getRawSample(&adc1, ADC_VOL_CH);
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

int8_t platform_getChSelector()
{
    return 0;    // TODO
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
    (void) freq;
}

void platform_beepStop()
{

}

const hwInfo_t *platform_getHwInfo()
{
    return &hwInfo;
}
