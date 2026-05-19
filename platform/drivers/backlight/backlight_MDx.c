/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "interfaces/platform.h"
#include "peripherals/gpio.h"
#include "hwconfig.h"
#include "backlight.h"

#if defined(PLATFORM_MDUV3x0) && defined(CONFIG_SCREEN_BRIGHTNESS)

/*
 * Interrupt-based software PWM for backlight dimming on MD-UV3x0.
 * On this family of devices the GPIO for backlight control is not connected to
 * any of the available timer compare output channels, thus making impossible to
 * implement an hardware-based PWM.
 * However, we provide a software-base backlight dimming for experimental purposes.
 */

/* Name of interrupt handler is mangled for C++ compatibility */
void _Z29TIM1_TRG_COM_TIM11_IRQHandlerv()
{
    if(TIM11->SR & TIM_SR_CC1IF)
    {
        gpio_clearPin(LCD_BKLIGHT); /* Clear pin on compare match */
    }

    if(TIM11->SR & TIM_SR_UIF)
    {
        gpio_setPin(LCD_BKLIGHT);   /* Set pin on counter reload */
    }

    TIM11->SR = 0;
}
#endif


void backlight_init()
{
    #if !defined(PLATFORM_MDUV3x0) && !defined(PLATFORM_DM1701)    /* MD-3x0 and MD-9600 */
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
    TIM8->CCMR1 |= TIM_CCMR1_OC1M_2
                |  TIM_CCMR1_OC1M_1
                |  TIM_CCMR1_OC1PE;
    TIM8->CCER  |= TIM_CCER_CC1E;
    TIM8->BDTR  |= TIM_BDTR_MOE;
    TIM8->CCR1 = 0;
    TIM8->EGR  = TIM_EGR_UG;        /* Update registers */
    TIM8->CR1 |= TIM_CR1_CEN;       /* Start timer */
    #else
    gpio_setMode(LCD_BKLIGHT, OUTPUT);
    gpio_clearPin(LCD_BKLIGHT);

    #ifdef CONFIG_SCREEN_BRIGHTNESS
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
    TIM11->CR1   |= TIM_CR1_ARPE;
    TIM11->CCMR1 |= TIM_CCMR1_OC1M_2
                 |  TIM_CCMR1_OC1M_1
                 |  TIM_CCMR1_OC1PE;
    TIM11->CCER  |= TIM_CCER_CC1E;
    TIM11->CCR1 = 0;
    TIM11->EGR  = TIM_EGR_UG;        /* Update registers            */
    TIM11->SR   = 0;                 /* Clear interrupt flags       */
    TIM11->DIER = TIM_DIER_CC1IE     /* Interrupt on compare match  */
                | TIM_DIER_UIE;      /* Interrupt on counter reload */
    TIM11->CR1 |= TIM_CR1_CEN;       /* Start timer                 */

    NVIC_ClearPendingIRQ(TIM1_TRG_COM_TIM11_IRQn);
    NVIC_SetPriority(TIM1_TRG_COM_TIM11_IRQn,15);
    NVIC_EnableIRQ(TIM1_TRG_COM_TIM11_IRQn);
    #endif
    #endif
}

void backlight_terminate()
{
    /* Shut down backlight */
    gpio_setMode(LCD_BKLIGHT, OUTPUT);
    gpio_clearPin(LCD_BKLIGHT);

    /* Shut down timer */
    RCC->APB2ENR &= ~RCC_APB2ENR_TIM8EN;
    RCC->APB2ENR &= ~RCC_APB2ENR_TIM11EN;
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

    #if !defined(PLATFORM_MDUV3x0) && !defined(PLATFORM_DM1701)
    TIM8->CCR1 = pwmLevel;
    #else
    /*
     * If CCR1 value is zero, a waveform with 99% duty cycle is generated: to
     * avoid this the PWM is cut off when backlight level is 1.
     */
    if(pwmLevel > 1)
    {
        #ifdef CONFIG_SCREEN_BRIGHTNESS
        TIM11->CCR1 = pwmLevel;
        TIM11->CR1 |= TIM_CR1_CEN;
        #else
        gpio_setPin(LCD_BKLIGHT);
        #endif
    }
    else
    {
        TIM11->CR1 &= ~TIM_CR1_CEN;
        gpio_clearPin(LCD_BKLIGHT);
    }
    #endif
}
