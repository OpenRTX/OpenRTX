/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

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
