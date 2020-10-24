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

#include <platform.h>
#include <gpio.h>
#include "hwconfig.h"
#include "adc1.h"

void platform_init()
{
    /* Configure GPIOs */
    gpio_setMode(GREEN_LED, OUTPUT);
    gpio_setMode(RED_LED, OUTPUT);

    /* Backlight pin connected to TIM8 CR1 */
    gpio_setMode(LCD_BKLIGHT, ALTERNATE);
    gpio_setAlternateFunction(LCD_BKLIGHT, 3);

    /* Initialise ADC1, for vbat, RSSI, ... */
    adc1_init();

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
}

void platform_terminate()
{
    /* Shut down backlight */
    gpio_setMode(LCD_BKLIGHT, OUTPUT);
    gpio_clearPin(LCD_BKLIGHT);

    gpio_clearPin(GREEN_LED);
    gpio_clearPin(RED_LED);

    /* Shut down timer */
    RCC->APB2ENR &= ~RCC_APB2ENR_TIM8EN;

    /* Shut down ADC */
    adc1_terminate();
}

float platform_getVbat()
{
    return adc1_getMeasurement(0);
}

float platform_getMicLevel()
{
    return adc1_getMeasurement(2);
}

float platform_getVolumeLevel()
{
    return adc1_getMeasurement(3);
}

uint8_t platform_getChSelector()
{
    return 0.0f;
}

bool platform_getPttStatus()
{
    return false;
}

void platform_ledOn(led_t led)
{
    switch(led)
    {
        case GREEN:
            gpio_setPin(GREEN_LED);
            break;

        case RED:
            gpio_setPin(RED_LED);
            break;

        default:
            break;
    }
}

void platform_ledOff(led_t led)
{
    switch(led)
    {
        case GREEN:
            gpio_clearPin(GREEN_LED);
            break;

        case RED:
            gpio_clearPin(RED_LED);
            break;

        default:
            break;
    }
}

void platform_beepStart(uint16_t freq)
{
    (void) freq;
}

void platform_beepStop()
{

}

void platform_setBacklightLevel(uint8_t level)
{
    TIM8->CCR1 = level;
}
