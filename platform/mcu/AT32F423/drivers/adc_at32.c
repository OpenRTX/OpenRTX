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

#include <at32f423.h>
#include <pthread.h>
#include "adc_at32.h"
#include <stdio.h>

uint16_t adcAt32_sample(const struct Adc *adc, const uint32_t channel);

int adcAt32_init(const struct Adc *adc)
{
    CRM->apb2en_bit.adc1en = TRUE;
    __DSB();
    CRM->misc1_bit.pllclk_to_adc = CRM_ADC_CLOCK_SOURCE_HCLK;

    adc_type *pAdc = ((adc_type *) adc->priv);

    ADCCOM->cctrl_bit.adcdiv = ADC_HCLK_DIV_4;
    ADCCOM->cctrl_bit.itsrven = FALSE;

    pAdc->ctrl1_bit.sqen = FALSE;
    pAdc->ctrl2_bit.rpen = FALSE;
    pAdc->ctrl2_bit.dtalign = ADC_RIGHT_ALIGNMENT;
    pAdc->osq1_bit.oclen = 2;
    pAdc->ctrl1_bit.crsel = ADC_RESOLUTION_12B;
    pAdc->misc = 0x09;
    pAdc->ctrl2_bit.adcen = TRUE;

    while(pAdc->sts_bit.rdy == 0) ;

    pAdc->ctrl2_bit.adcal = TRUE;
    while(pAdc->ctrl2_bit.adcal) ;

    if(adc->mutex != NULL)
        pthread_mutex_init((pthread_mutex_t *) adc->mutex, NULL);

    return 0;
}

void adcAt32_terminate(const struct Adc *adc)
{
    /* A conversion may be in progress, wait until it finishes */
    if(adc->mutex != NULL)
        pthread_mutex_lock((pthread_mutex_t *) adc->mutex);


    if(adc->mutex != NULL)
        pthread_mutex_destroy((pthread_mutex_t *) adc->mutex);
}

uint16_t adcAt32_sample(const struct Adc *adc, const uint32_t channel)
{
    if(channel > 27)
        return 0;

    adc_type *pAdc = ((adc_type *) adc->priv);

    /* Channel 16 is internal temperature */
    if(channel == 16)
        ((adccom_type*)ADCCOM_BASE)->cctrl_bit.itsrven = TRUE;
    /* Channel 17 is internal reference voltage */
    if(channel == 17)
        ((adccom_type*)ADCCOM_BASE)->cctrl_bit.itsrven = TRUE;

    if(channel < ADC_CHANNEL_10)
    {
        pAdc->spt2 = (pAdc->spt2 & ~(0x07 << 3 * channel)) | (ADC_SAMPLETIME_12_5 << 3 * channel);
    }
    else if(channel < ADC_CHANNEL_20)
    {
        pAdc->spt1 = (pAdc->spt1 & ~(0x07 << (3 * (channel - ADC_CHANNEL_10)))) | (ADC_SAMPLETIME_12_5 << (3 * (channel - ADC_CHANNEL_10)));
    }
    else
    {
        pAdc->spt3 = (pAdc->spt3 & ~(0x07 << (3 * (channel - ADC_CHANNEL_20)))) | (ADC_SAMPLETIME_12_5 << (3 * (channel - ADC_CHANNEL_20)));
    }
    pAdc->osq3_bit.osn1 = channel;
    pAdc->ctrl2_bit.octesel = ADC_PREEMPT_TRIG_SOFTWARE;
    pAdc->ctrl2_bit.ocswtrg = TRUE;

    while((pAdc->sts_bit.occe) == 0) ;

    uint16_t value = pAdc->odt_bit.odt;

    /* Disconnect Vbat channel. Vbat has an internal x2 voltage divider */
    if(channel == 16 || channel == 17)
    {
        value *= 2;
        ((adccom_type*)ADCCOM_BASE)->cctrl_bit.itsrven = FALSE;
    }

    return (uint16_t) value;
}
