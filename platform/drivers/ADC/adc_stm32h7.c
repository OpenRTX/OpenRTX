/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "stm32h7xx.h"
#include <pthread.h>
#include <errno.h>
#include "adc_stm32.h"

int adcStm32_init(const struct Adc *adc)
{
    /*
     * Configure ADC for synchronous clock mode (clocked by AHB clock), divided
     * by 4. This gives an ADC clock of 50MHz  when AHB clock is 200MHz.
     *
     * NOTE: ADC1 and ADC2 share the same clock tree!
     */

    switch((uint32_t) adc->priv)
    {
        case ADC1_BASE:
        case ADC2_BASE:
            RCC->AHB1ENR |= RCC_AHB1ENR_ADC12EN;
            __DSB();
            ADC12_COMMON->CCR = ADC_CCR_CKMODE_1
                              | ADC_CCR_CKMODE_0;
            break;

        case ADC3_BASE:
            RCC->AHB4ENR |= RCC_AHB4ENR_ADC3EN;
            __DSB();
            ADC3_COMMON->CCR = ADC_CCR_CKMODE_1
                             | ADC_CCR_CKMODE_0;
            break;

        default:
            return -EINVAL;
            break;
    }

    ADC_TypeDef *pAdc = ((ADC_TypeDef *) adc->priv);

    // Enable ADC voltage regulator, enable boost mode. Wait until LDO regulator
    // is ready.
    pAdc->CR = ADC_CR_ADVREGEN
             | ADC_CR_BOOST_1
             | ADC_CR_BOOST_0;

    while((pAdc->ISR & ADC_ISR_LDORDY) == 0) ;

    // Start calibration, both offset and linearity.
    pAdc->CR |= ADC_CR_ADCAL
             |  ADC_CR_ADCALLIN;

    while((pAdc->CR & ADC_CR_ADCAL) != 0) ;

    /*
     * ADC clock is 50MHz. We set the sample time of each channel to 387.5 ADC
     * cycles, giving a total conversion time of ~7us.
     */
    pAdc->SMPR2 = 0x36DB6DB6;
    pAdc->SMPR1 = 0x36DB6DB6;

    // Finally,turn on the ADC
    pAdc->CR   |= ADC_CR_ADEN;
    while((pAdc->ISR & ADC_ISR_ADRDY) != 0) ;

    if(adc->mutex != NULL)
        pthread_mutex_init((pthread_mutex_t *) adc->mutex, NULL);

    return 0;
}

void adcStm32_terminate(const struct Adc *adc)
{
    // A conversion may be in progress, wait until it finishes
    if(adc->mutex != NULL)
        pthread_mutex_lock((pthread_mutex_t *) adc->mutex);

    ((ADC_TypeDef *) adc->priv)->CR = 0;

    switch((uint32_t) adc->priv)
    {
        case ADC1_BASE:
        case ADC2_BASE:
            if((ADC1->CR == 0) && (ADC2->CR == 0))
                RCC->AHB1ENR &= ~RCC_AHB1ENR_ADC12EN;
            __DSB();
            break;

        case ADC3_BASE:
            RCC->AHB4ENR &= ~RCC_AHB4ENR_ADC3EN;
            __DSB();
            break;

        default:
            break;
    }

    if(adc->mutex != NULL)
        pthread_mutex_destroy((pthread_mutex_t *) adc->mutex);
}

uint16_t adcStm32_sample(const struct Adc *adc, const uint32_t channel)
{
    if(channel > 16)
        return 0;

    ADC_TypeDef *pAdc = ((ADC_TypeDef *) adc->priv);

    pAdc->SQR1 = channel << ADC_SQR1_SQ1_Pos;
    pAdc->PCSEL = 1 << channel;
    pAdc->CR |= ADC_CR_ADSTART;

    while((pAdc->ISR & ADC_ISR_EOC) == 0) ;

    return pAdc->DR;
}
