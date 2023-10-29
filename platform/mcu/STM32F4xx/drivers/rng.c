/***************************************************************************
 *   Copyright (C) 2023 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
 *                         Frederik Saraci IU2NRO                          *
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
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#include <interfaces/delays.h>
#include <peripherals/rng.h>
#include <stm32f4xx.h>
#include <pthread.h>

static uint32_t oldValue = 0;

void rng_init()
{
    RCC->AHB2ENR |= RCC_AHB2ENR_RNGEN;
    __DSB();
}

void rng_terminate()
{
    RCC->AHB2ENR &= ~RCC_AHB2ENR_RNGEN;
    __DSB();
}

uint32_t rng_get()
{
    uint32_t value;

    RNG->CR = RNG_CR_RNGEN;

    /*
     * The datasheet says that the RNG takes 40 clock cycles of the 48MHz
     * clock to generate a number, that is around 0.83us. It has not much
     * sense to put a delay here, we can just spin a bit and wait.
     */
    while(1)
    {
        uint32_t status = RNG->SR;

        // Got a new number, return it only if different from the previous one
        if((status & RNG_SR_DRDY) != 0)
        {
            value = RNG->DR;
            if(value != oldValue)
                break;
        }

        // Error, "power cycle" the RNG and retry
        if((status & (RNG_SR_SECS | RNG_SR_CECS)) != 0)
        {
            RNG->CR = 0;
            delayUs(1);
            RNG->CR = RNG_CR_RNGEN;
        }
    }

    RNG->CR = 0;
    oldValue = value;

    return value;
}
