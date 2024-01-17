/***************************************************************************
 *   Copyright (C) 2020 - 2023 by Silvano Seva IU2KWO                      *
 *                            and Niccolò Izzo IU2KIN                      *
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

#include <peripherals/gpio.h>
#include <hwconfig.h>
#include <pthread.h>
#include <stdlib.h>
#include "ADC1_Mod17.h"

static pthread_mutex_t adcMutex;

void adc1_init()
{
    pthread_mutex_init(&adcMutex, NULL);

    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
    __DSB();

    /*
     * ADC clock is APB2 frequency divided by 8, giving 10.5MHz.
     * We set the sample time of each channel to 84 ADC cycles and we have that
     * a conversion takes 12 cycles: total conversion time is then of ~9us.
     */
    ADC->CCR   |= ADC_CCR_ADCPRE;
    ADC1->SMPR2 = ADC_SMPR2_SMP3_2;

    /*
     * Convert one channel, no overrun interrupt, 12-bit resolution,
     * no analog watchdog, discontinuous mode, no end of conversion interrupts,
     * turn on ADC.
     */
    ADC1->SQR1 = 0;
    ADC1->CR2  = ADC_CR2_ADON;
}

void adc1_terminate()
{
    pthread_mutex_destroy(&adcMutex);

    ADC1->CR2    &= ~ADC_CR2_ADON;
    RCC->APB2ENR &= ~RCC_APB2ENR_ADC1EN;
    __DSB();
}

uint16_t adc1_getRawSample(uint8_t ch)
{
    if(ch > 15) return 0;

    pthread_mutex_lock(&adcMutex);

    ADC1->SQR3 = ch;
    ADC1->CR2 |= ADC_CR2_SWSTART;
    while((ADC1->SR & ADC_SR_EOC) == 0) ;
    uint16_t value = ADC1->DR;

    pthread_mutex_unlock(&adcMutex);

    return value;
}

uint16_t adc1_getMeasurement(uint8_t ch)
{
    /*
     * To avoid using floats, we convert the raw ADC sample to mV using 16.16
     * fixed point math. The equation for conversion is (sample * 3300)/4096 but,
     * since converting the raw ADC sample to 16.16 notation requires a left
     * shift by 16 and dividing by 4096 is equivalent to shifting right by 12,
     * we just shift left by four and then multiply by 3300.
     * With respect to using floats, maximum error is -1mV.
     */
    uint32_t sample = (adc1_getRawSample(ch) << 4) * 3300;
    return ((uint16_t) (sample >> 16));
}
