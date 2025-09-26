/***************************************************************************
 *   Copyright (C) 2022 - 2025 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN                      *
 *                                Frederik Saraci IU2NRO                   *
 *                                Silvano Seva IU2KWO                      *
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

#ifndef TIMERS_H
#define TIMERS_H

#include <stdint.h>
#include "stm32f4xx.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Configure prescaler and auto-reload register of a timer peripheral for a
 * given update frequency.
 *
 * @param tim: timer peripheral to be configured.
 * @param updFreq: desidered update frequency, in Hz.
 * @param busFreq: frequency of the APB bus the timer is attached to, in Hz.
 * @return effective timer update frequency, in Hz.
 */
inline uint32_t tim_setUpdateFreqency(TIM_TypeDef *tim, uint32_t updFreq,
                                      uint32_t busFreq)
{
    /*
     * Timer update frequency is given by:
     * Fupd = (Fbus / prescaler) / autoreload
     *
     * First of all we fix the prescaler to 1 and compute the autoreload: if
     * the result is greater than the maximum autoreload value, we proceed
     * iteratively.
     */
    uint32_t psc = 1;
    uint32_t arr = busFreq/updFreq;

    while(arr >= 0xFFFF)
    {
        psc += 1;
        arr = (busFreq / psc) / updFreq;
    }

    /* Values put in registers have to be decremented by one, see RM */
    tim->PSC = psc - 1;
    tim->ARR = arr - 1;

    return (busFreq/psc)/arr;
}


#ifdef __cplusplus
}
#endif

#endif /* USART3_H */
