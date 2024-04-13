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
#include <gpio_shiftReg.h>
#include <spi_bitbang.h>
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
    gpio_setMode(PTT_SW,   INPUT);
    gpio_setMode(PTT_EXT,  INPUT);

    gpio_setMode(GPIOEXT_CLK, OUTPUT);
    gpio_setMode(GPIOEXT_DAT, OUTPUT);

    spi_init((const struct spiDevice *) &spiSr);
    gpioShiftReg_init(&extGpio);

    #ifndef RUNNING_TESTSUITE
    gpioDev_set(MAIN_PWR_SW);
    #endif
}

void platform_terminate()
{

    #ifndef RUNNING_TESTSUITE
    gpioDev_clear(MAIN_PWR_SW);
    #endif

    gpioShiftReg_terminate(&extGpio);
}

uint16_t platform_getVbat()
{
    return 7400;   // TODO
}

uint8_t platform_getMicLevel()
{
    return 0;   // TODO
}

uint8_t platform_getVolumeLevel()
{
    return 0;   // TODO
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
    return (platform_getVbat() > 1000) ? true : false;
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
