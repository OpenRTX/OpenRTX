/***************************************************************************
 *   Copyright (C) 2020 by Silvano Seva IU2KWO and Niccolò Izzo IU2KIN     *
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

#include "pll_MD3x0.h"
#include <math.h>
#include "gpio.h"
#include "delays.h"
#include "hwconfig.h"

#define REF_CLK 16800000.0F  /* Reference clock: 16.8MHz                 */
#define PHD_GAIN 0x1F        /* Phase detector gain: hex value, max 0x1F */

void _spiSend(uint16_t value)
{
    uint16_t temp = value;

    gpio_clearPin(PLL_CS);
    delayUs(10);

    for(uint8_t i = 0; i < 16; i++)
    {
        gpio_setPin(PLL_CLK);
        delayUs(1);

        if(temp & 0x8000)
        {
            gpio_setPin(PLL_DAT);
        }
        else
        {
            gpio_clearPin(PLL_DAT);
        }

        temp <<= 1;

        delayUs(1);
        gpio_clearPin(PLL_CLK);
        delayUs(1);
    }

    gpio_setPin(PLL_CLK);

    delayUs(10);
    gpio_setPin(PLL_CS);
}

void pll_init()
{
    gpio_setMode(PLL_CLK, OUTPUT);
    gpio_setMode(PLL_DAT, OUTPUT);
    gpio_setMode(PLL_CS,  OUTPUT);
    gpio_setPin(PLL_CS);
    gpio_setMode(PLL_LD, INPUT);

    _spiSend(0x6000 | ((uint16_t) PHD_GAIN)); /* Phase detector gain                     */
    _spiSend(0x73D0);                         /* Power down/multiplexer control register */
    _spiSend(0x8000);                         /* Modulation control register             */
    _spiSend(0x9000);                         /* Modulation data register                */
}

void pll_terminate()
{
    gpio_setMode(PLL_CLK, INPUT);
    gpio_setMode(PLL_DAT, INPUT);
    gpio_setMode(PLL_CS,  INPUT);
}

void pll_setFrequency(float freq, uint8_t clkDiv)
{
    /* Maximum allowable value for reference clock divider is 32 */
    if (clkDiv > 32) clkDiv = 32;

    float K = freq/(REF_CLK/((float) clkDiv));
    float Ndiv = floor(K) - 32.0;
    float Ndnd = round(262144*(K - Ndiv - 32.0));

    /*
     * With PLL in fractional mode, dividend range is [-131017 +131071]. If our
     * computation gives a wrong result, we decrement the reference clock divider
     * and redo the computations.
     *
     * TODO: better investigate on how to put PLL in unsigned dividend mode.
     */
    if(((uint32_t) Ndnd) >= 131070)
    {
        clkDiv -= 1;
        K = freq/(REF_CLK/((float) clkDiv));
        Ndiv = floor(K) - 32.0;
        Ndnd = round(262144*(K - Ndiv - 32.0));

    }

    uint32_t dnd = ((uint32_t) Ndnd);
    uint16_t dndMsb = dnd >> 8;
    uint16_t dndLsb = dnd & 0x00FF;

    _spiSend((uint16_t) Ndiv);                   /* Divider register      */
    _spiSend(0x2000 | dndLsb);                   /* Dividend LSB register */
    _spiSend(0x1000 | dndMsb);                   /* Dividend MSB register */
    _spiSend(0x5000 | ((uint16_t)clkDiv - 1));   /* Reference clock divider */
}

bool pll_locked()
{
    return (gpio_readPin(PLL_LD) == 1) ? true : false;
}

bool pll_spiInUse()
{
    /* If PLL chip select is low, SPI is being used by this driver. */
    return (gpio_readPin(PLL_CS) == 1) ? false : true;
}

