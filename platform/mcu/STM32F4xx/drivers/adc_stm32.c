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

#include <stm32f4xx.h>
#include <pthread.h>
#include <errno.h>
#include "adc_stm32.h"

int adcStm32_init(const struct Adc *adc)
{
    switch((uint32_t) adc->priv)
    {
        case ADC1_BASE:
            RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
            __DSB();
            break;

        case ADC2_BASE:
            RCC->APB2ENR |= RCC_APB2ENR_ADC2EN;
            __DSB();
            break;

        case ADC3_BASE:
            RCC->APB2ENR |= RCC_APB2ENR_ADC3EN;
            __DSB();
            break;

        default:
            return -EINVAL;
            break;
    }

    ADC_TypeDef *pAdc = ((ADC_TypeDef *) adc->priv);

    /*
     * ADC clock is APB2 frequency divided by 8, giving 10.5MHz.
     * We set the sample time of each channel to 84 ADC cycles and we have that
     * a conversion takes 12 cycles: total conversion time is then of ~9us.
     */
    ADC->CCR   |= ADC_CCR_ADCPRE;
    pAdc->SMPR2 = 0x4924924;
    pAdc->SMPR1 = 0x24924924;

    /*
     * Convert one channel, no overrun interrupt, 12-bit resolution,
     * no analog watchdog, discontinuous mode, no end of conversion interrupts,
     * turn on ADC.
     */
    pAdc->SQR1 = 0;
    pAdc->CR2  = ADC_CR2_ADON;

    if(adc->mutex != NULL)
        pthread_mutex_init((pthread_mutex_t *) adc->mutex, NULL);

    return 0;
}

void adcStm32_terminate(const struct Adc *adc)
{
    /* A conversion may be in progress, wait until it finishes */
    if(adc->mutex != NULL)
        pthread_mutex_lock((pthread_mutex_t *) adc->mutex);

    ((ADC_TypeDef *) adc->priv)->CR2 = 0;

    switch((uint32_t) adc->priv)
    {
        case ADC1_BASE:
            RCC->APB2ENR &= ~RCC_APB2ENR_ADC1EN;
            __DSB();
            break;

        case ADC2_BASE:
            RCC->APB2ENR &= ~RCC_APB2ENR_ADC2EN;
            __DSB();
            break;

        case ADC3_BASE:
            RCC->APB2ENR &= ~RCC_APB2ENR_ADC3EN;
            __DSB();
            break;
    }

    if(adc->mutex != NULL)
        pthread_mutex_destroy((pthread_mutex_t *) adc->mutex);
}

uint16_t adcStm32_sample(const struct Adc *adc, const uint32_t channel)
{
    if(channel > 19)
        return 0;

    ADC_TypeDef *pAdc = ((ADC_TypeDef *) adc->priv);

    /* Channel 18 is Vbat, enable it if requested */
    if(channel == 18)
        ADC->CCR |= ADC_CCR_VBATE;

    pAdc->SQR3 = channel;
    pAdc->CR2 |= ADC_CR2_SWSTART;

    while((pAdc->SR & ADC_SR_EOC) == 0) ;

    uint16_t value = pAdc->DR;

    /* Disconnect Vbat channel. Vbat has an internal x2 voltage divider */
    if(channel == 18)
    {
        value *= 2;
        ADC->CCR &= ~ADC_CCR_VBATE;
    }

    return value;
}
