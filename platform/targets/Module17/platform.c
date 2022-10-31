/***************************************************************************
 *   Copyright (C) 2021 - 2022 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN,                     *
 *                                Frederik Saraci IU2NRO,                  *
 *                                Silvano Seva IU2KWO                      *
 *                                Mathis Schmieder DB9MAT                  *
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
#include <backlight.h>
#include <interfaces/rtc.h>
#include <interfaces/audio.h>
#include <ADC1_Mod17.h>
#include <interfaces/nvmem.h>
#include <MCP4551.h>

void platform_init()
{
    /* Configure GPIOs */
    gpio_setMode(PTT_LED,  OUTPUT);
    gpio_setMode(SYNC_LED, OUTPUT);
    gpio_setMode(ERR_LED,  OUTPUT);

    gpio_setMode(PTT_SW,  INPUT);
    gpio_setMode(PTT_OUT, OUTPUT);
    gpio_clearPin(PTT_OUT);

    /* Set analog output for baseband signal to an idle level of 1.1V */
    gpio_setMode(BASEBAND_TX, INPUT_ANALOG);
    RCC->APB1ENR |= RCC_APB1ENR_DACEN;
    DAC->CR      |= DAC_CR_EN1;
    DAC->DHR12R1  = 1365;

    nvm_init();
    adc1_init();
    i2c_init();
    mcp4551_init(SOFTPOT_RX);
    mcp4551_init(SOFTPOT_TX);

    audio_init();
}

void platform_terminate()
{
    /* Shut down LEDs */
    gpio_clearPin(PTT_LED);
    gpio_clearPin(SYNC_LED);
    gpio_clearPin(ERR_LED);

    adc1_terminate();
    nvm_terminate();
    audio_terminate();
}

uint16_t platform_getVbat()
{
   return adc1_getMeasurement(ADC_VBAT_CH)*5;
}

uint8_t platform_getMicLevel()
{
    return 0;
}

uint8_t platform_getVolumeLevel()
{
    return 0;
}

int8_t platform_getChSelector()
{
    return 0;
}

bool platform_getPttStatus()
{
    /* PTT line has a pullup resistor with PTT switch closing to ground */
    return (gpio_readPin(PTT_SW) == 0) ? true : false;
}

bool platform_pwrButtonStatus()
{
    return true;
}

void platform_ledOn(led_t led)
{
    switch(led)
    {
        case RED:
            gpio_setPin(PTT_LED);
            break;

        case GREEN:
            gpio_setPin(SYNC_LED);
            break;

        case YELLOW:
            gpio_setPin(ERR_LED);
            break;

        default:
            break;
    }
}

void platform_ledOff(led_t led)
{
    switch(led)
    {
        case RED:
            gpio_clearPin(PTT_LED);
            break;

        case GREEN:
            gpio_clearPin(SYNC_LED);
            break;

        case YELLOW:
            gpio_clearPin(ERR_LED);
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
    return NULL;
}

const hwInfo_t *platform_getHwInfo()
{
    return NULL;
}

void platform_setBacklightLevel(uint8_t level)
{
    (void) level;
}
