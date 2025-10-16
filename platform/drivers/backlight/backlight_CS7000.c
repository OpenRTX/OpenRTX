/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "interfaces/platform.h"
#include "peripherals/gpio.h"
#include "hwconfig.h"
#include "backlight.h"

void backlight_init()
{
    gpioDev_clear(KBD_BKLIGHT);
    gpio_setMode(LCD_BKLIGHT, ALTERNATE | ALTERNATE_FUNC(3));

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
    TIM8->CR1   |= TIM_CR1_ARPE;    /* LCD backlight is on PC6, TIM8-CH1 */
    TIM8->CCMR2 |= TIM_CCMR1_OC2M_2
                |  TIM_CCMR1_OC2M_1
                |  TIM_CCMR1_OC2PE;
    TIM8->CCER  |= TIM_CCER_CC4E;
    TIM8->BDTR  |= TIM_BDTR_MOE;
    TIM8->CCR1 = 0;
    TIM8->EGR  = TIM_EGR_UG;        /* Update registers */
    TIM8->CR1 |= TIM_CR1_CEN;       /* Start timer */
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
 * This function is defined in display.h
 */
void display_setBacklightLevel(uint8_t level)
{
    if(level > 100)
        level = 100;

    uint8_t pwmLevel = (2 * level) + (level * 55)/100;    // Convert value to 0 - 255
    TIM8->CCR4 = pwmLevel;

    // Keyboard backlight does not have dimming, only on/off
    if(level > 0)
        gpioDev_set(KBD_BKLIGHT);
    else
        gpioDev_clear(KBD_BKLIGHT);
}
