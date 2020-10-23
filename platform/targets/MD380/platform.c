/***************************************************************************
 *   Copyright (C) 2020 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
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

#include <stm32f4xx.h>
#include "platform.h"
#include "gpio.h"

void platform_init()
{
    /*
     * Configure TIM8 for backlight PWM: Fpwm = 100kHz, 8 bit of resolution
     * APB2 freq. is 84MHz, then: PSC = 327 to have Ftick = 256.097kHz
     * With ARR = 256, Fpwm is 100kHz;
     */
    RCC->APB2ENR |= RCC_APB2ENR_TIM8EN;
    TIM8->ARR = 255;
    TIM8->PSC = 327;
    TIM8->CNT = 0;
    TIM8->CR1   |= TIM_CR1_ARPE;    /* LCD backlight is on PC6, TIM8-CH1 */
    TIM8->CCMR1 |= TIM_CCMR1_OC1M_2
                |  TIM_CCMR1_OC1M_1
                |  TIM_CCMR1_OC1PE;
    TIM8->CCER  |= TIM_CCER_CC1E;
    TIM8->BDTR  |= TIM_BDTR_MOE;
    TIM8->CCR1 = 0;
    TIM8->EGR  = TIM_EGR_UG;        /* Update registers */
    TIM8->CR1 |= TIM_CR1_CEN;       /* Start timer */

    /* Configure backlight GPIO, TIM8 is on AF3 */
    gpio_setMode(LCD_BKLIGHT, ALTERNATE);
    gpio_setAlternateFunction(LCD_BKLIGHT, 3);
}

void platform_terminate()
{
    /* Shut off backlight */
    gpio_setMode(LCD_BKLIGHT, OUTPUT);
    gpio_clearPin(LCD_BKLIGHT);
    RCC->APB2ENR &= ~RCC_APB2ENR_TIM8EN;
}

void platform_setBacklightLevel(uint8_t level)
{
    TIM8->CCR1 = level;
}
