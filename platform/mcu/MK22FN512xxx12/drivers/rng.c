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

#include <peripherals/rng.h>
#include <MK22F51212.h>

void rng_init()
{
    SIM->SCGC6 |= SIM_SCGC6_RNGA(1);
    RNG->CR    |= RNG_CR_GO(1);
}

void rng_terminate()
{
    SIM->SCGC6 &= ~SIM_SCGC6_RNGA(1);
}

uint32_t rng_get()
{
    // Wait until there is some data
    while((RNG->SR & RNG_SR_OREG_LVL_MASK) == 0) ;

    return RNG->OR;
}
