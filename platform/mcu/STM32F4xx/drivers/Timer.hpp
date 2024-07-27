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

#ifndef TIMER_H
#define TIMER_H

#include <stm32f4xx.h>

/**
 * Handler class for STM32F4 Timer peripheral.
 */
class Timer
{
public:

    /**
     * Constructor.
     *
     * @param tim: base address of timer peripheral to manage.
     */
    constexpr Timer(const uint32_t tim) : tim(tim) {}

    ~Timer() = default;

    /**
     * Configure timer prescaler and auto-reload registers for a given update
     * frequency.
     *
     * @param busFreq: frequency of the APB bus the timer is attached to, in Hz.
     * @param updFreq: desidered update frequency, in Hz.
     * @return effective timer update frequency, in Hz.
     */
    uint32_t setUpdateFrequency(const uint32_t busFreq, const uint32_t updFreq) const
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
        uint32_t arr = busFreq / updFreq;
        while(arr >= 0xFFFF)
        {
            psc += 1;
            arr = (busFreq / psc) / updFreq;
        }

        // Values put in registers have to be decremented by one, see RM
        reinterpret_cast< TIM_TypeDef * >(tim)->PSC = psc - 1;
        reinterpret_cast< TIM_TypeDef * >(tim)->ARR = arr - 1;
        reinterpret_cast< TIM_TypeDef * >(tim)->CR2 = TIM_CR2_MMS_1;

        return (busFreq / psc) / arr;
    }

    /**
     * Configure timer prescaler and auto-reload registers for a given update
     * frequency.
     *
     * @param updFreq: desidered update frequency, in Hz.
     * @return effective timer update frequency, in Hz.
     */
    uint32_t setUpdateFrequency(const uint32_t updFreq) const
    {
        /*
         * Compute the frequency of the bus the timer is attached to.
         * This formula takes into account that if the APB1 clock is divided by
         * a factor of two or greater, the timer is clocked at twice the bus
         * interface.
         */
        uint32_t busFreq = SystemCoreClock;
        uint32_t APB     = (tim & 0x40010000) >> 16;
        uint32_t shift   = 10U << (3 * APB);

        if(RCC->CFGR & RCC_CFGR_PPRE1_2)
            busFreq /= 1 << ((RCC->CFGR >> shift) & 0x3);

        return setUpdateFrequency(busFreq, updFreq);
    }

    /**
     * Enable or disable the generation of DMA requests on timer update events.
     *
     * @param enable: set to true to enable the generation of DMA requests.
     */
    inline void enableDmaTrigger(const bool enable) const
    {
        if(enable)
            reinterpret_cast< TIM_TypeDef * >(tim)->DIER = TIM_DIER_UDE;
        else
            reinterpret_cast< TIM_TypeDef * >(tim)->DIER = 0;
    }

    /**
     * Clear and start the timer's counter.
     */
    inline void start() const
    {
        reinterpret_cast< TIM_TypeDef * >(tim)->CNT = 0;
        reinterpret_cast< TIM_TypeDef * >(tim)->EGR = TIM_EGR_UG;
        reinterpret_cast< TIM_TypeDef * >(tim)->CR1 = TIM_CR1_CEN;
    }

    /**
     * Stop the timer's counter.
     */
    inline void stop() const
    {
        reinterpret_cast< TIM_TypeDef * >(tim)->CR1 = 0;
    }

    /**
     * Get current value of timer's counter.
     *
     * @return value of timer's counter.
     */
    inline uint16_t value() const
    {
        return reinterpret_cast< TIM_TypeDef * >(tim)->CNT = 0;
    }

private:

    const uint32_t tim;
};

#endif /* TIMER_H */
