/***************************************************************************
 *   Copyright (C) 2021 by Federico Amedeo Izzo IU2NUO,                    *
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

#include <hwconfig.h>
#include <interfaces/gpio.h>
#include <interfaces/platform.h>

#include "backlight.h"

#ifndef PLATFORM_MDUV3x0 /* MD-3x0 and MD-9600 */

void backlight_init()
{
    gpio_setMode(LCD_BKLIGHT, ALTERNATE);
    gpio_setAlternateFunction(LCD_BKLIGHT, 3);

    /*
     * Configure TIM8 for backlight PWM: Fpwm = 1kHz with 8 bit of resolution.
     * APB2 freq. is 84MHz, but timer runs at twice this frequency.
     * Then: PSC = 655 to have Ftick = 256.097kHz
     * With ARR = 256, Fpwm is 1kHz;
     * Backlight pin is connected to TIM8 CR1.
     */
    RCC->APB2ENR |= RCC_APB2ENR_TIM8EN;
    __DSB();

    TIM8->ARR = 255;
    TIM8->PSC = 654;
    TIM8->CNT = 0;
    TIM8->CR1 |= TIM_CR1_ARPE; /* LCD backlight is on PC6, TIM8-CH1 */
    TIM8->CCMR1 |= TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1PE;
    TIM8->CCER |= TIM_CCER_CC1E;
    TIM8->BDTR |= TIM_BDTR_MOE;
    TIM8->CCR1 = 0;
    TIM8->EGR  = TIM_EGR_UG;  /* Update registers */
    TIM8->CR1 |= TIM_CR1_CEN; /* Start timer */
}

void backlight_terminate()
{
    /* Shut down backlight */
    gpio_setMode(LCD_BKLIGHT, OUTPUT);
    gpio_clearPin(LCD_BKLIGHT);

    /* Shut down timer */
    RCC->APB2ENR &= ~RCC_APB2ENR_TIM8EN;
    __DSB();
}

/*
 * This function is defined in platform.h
 */
void platform_setBacklightLevel(uint8_t level)
{
    TIM8->CCR1 = level;
}

#elif defined(ENABLE_BKLIGHT_DIMMING) /* MD-UV3x0 AND dimming enabled */

/*
 * Interrupt-based software PWM for backlight dimming on MD-UV3x0.
 * On this family of devices the GPIO for backlight control is not connected to
 * any of the available timer compare output channels, thus making impossible to
 * implement an hardware-based PWM.
 * However, we provide a software-base backlight dimming for experimental
 * purposes.
 */

/* Name of interrupt handler is mangled for C++ compatibility */
void _Z29TIM1_TRG_COM_TIM11_IRQHandlerv()
{
    if (TIM11->SR & TIM_SR_CC1IF)
    {
        gpio_clearPin(LCD_BKLIGHT); /* Clear pin on compare match */
    }

    if (TIM11->SR & TIM_SR_UIF)
    {
        gpio_setPin(LCD_BKLIGHT); /* Set pin on counter reload */
    }

    TIM11->SR = 0;
}

void backlight_init()
{
    gpio_setMode(LCD_BKLIGHT, OUTPUT);
    gpio_clearPin(LCD_BKLIGHT);

    /*
     * Configure TIM11 for backlight PWM: Fpwm = 256Hz, 8 bit of resolution.
     * APB2 freq. is 84MHz but timer runs at twice this frequency, then:
     * PSC = 2564 to have Ftick = 65.52kHz
     * With ARR = 256, Fpwm is 256Hz;
     */
    RCC->APB2ENR |= RCC_APB2ENR_TIM11EN;
    __DSB();

    TIM11->ARR = 255;
    TIM11->PSC = 2563;
    TIM11->CNT = 0;
    TIM11->CR1 |= TIM_CR1_ARPE;
    TIM11->CCMR1 |= TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1PE;
    TIM11->CCER |= TIM_CCER_CC1E;
    TIM11->CCR1 = 0;
    TIM11->EGR  = TIM_EGR_UG;     /* Update registers            */
    TIM11->SR   = 0;              /* Clear interrupt flags       */
    TIM11->DIER = TIM_DIER_CC1IE  /* Interrupt on compare match  */
                  | TIM_DIER_UIE; /* Interrupt on counter reload */
    TIM11->CR1 |= TIM_CR1_CEN;    /* Start timer                 */

    NVIC_ClearPendingIRQ(TIM1_TRG_COM_TIM11_IRQn);
    NVIC_SetPriority(TIM1_TRG_COM_TIM11_IRQn, 15);
    NVIC_EnableIRQ(TIM1_TRG_COM_TIM11_IRQn);
}

void backlight_terminate()
{
    /* Shut down backlight */
    gpio_clearPin(LCD_BKLIGHT);

    /* Shut down timer */
    RCC->APB2ENR &= ~RCC_APB2ENR_TIM11EN;
    __DSB();
}

/*
 * This function is defined in platform.h
 */
void platform_setBacklightLevel(uint8_t level)
{
    /*
     * Little workaround for the following nasty behaviour: if CCR1 value is
     * zero, a waveform with 99% duty cycle is generated. This is because we are
     * emulating pwm with interrupts.
     */
    if (level > 1)
    {
        TIM11->CCR1 = level;
        TIM11->CR1 |= TIM_CR1_CEN;
    }
    else
    {
        TIM11->CR1 &= ~TIM_CR1_CEN;
        gpio_clearPin(LCD_BKLIGHT);
    }
}

#endif
