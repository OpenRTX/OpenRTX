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

#include "pll_MD3x0.h"
#include <math.h>
#include <interfaces/gpio.h>
#include <interfaces/delays.h>
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

void pll_setFrequency(uint32_t freq, uint8_t clkDiv)
{
    /* temporary workaround, this fixes the +1kHz frequency offset at TX */
    freq-=1000;

    /* Maximum allowable value for reference clock divider is 32 */
    if (clkDiv > 32) clkDiv = 32;

    float K = freq/(REF_CLK/((float) clkDiv));
    float Ndiv = floor(K) - 32.0;
    float Ndnd = round(262144*(K - Ndiv - 32.0));

    /*
     * With PLL in fractional mode, dividend range is [-131017 +131071].
     * When converting from float to uint32_t we have to cast the value to a
     * signed 18-bit one and increment the divider by one if dividend is negative.
     */
    uint32_t dnd = ((uint32_t) Ndnd) & 0x03FFFF;
    if(dnd & 0x20000) Ndiv += 1;

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

