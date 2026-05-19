/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "hwconfig.h"
#include "ADC0_GDx.h"

void adc0_init()
{
    SIM->SCGC6 |= SIM_SCGC6_ADC0(1);

    ADC0->CFG1 |= ADC_CFG1_ADIV(3)      /* Divide clock by 8 */
               |  ADC_CFG1_ADLSMP(1)    /* Long sample time  */
               |  ADC_CFG1_MODE(3);     /* 16 bit result     */

    ADC0->SC3  |= ADC_SC3_AVGE(1)       /* Enable hardware average */
               |  ADC_SC3_AVGS(3);      /* Average over 32 samples */

    /* Calibrate ADC */
    ADC0->SC3 |= ADC_SC3_CAL(1);

    while((ADC0->SC3) & ADC_SC3_CAL_MASK) ;

    uint32_t cal = ADC0->CLP0
                 + ADC0->CLP1
                 + ADC0->CLP2
                 + ADC0->CLP3
                 + ADC0->CLP4
                 + ADC0->CLPS;

    ADC0->PG = 0x8000 | (cal >> 1);
}

void adc0_terminate()
{
    SIM->SCGC6 &= ~SIM_SCGC6_ADC0(1);
}

uint16_t adc0_getRawSample(uint8_t ch)
{
    if(ch > 32) return 0;

    /* Conversion is automatically initiated by writing to this register */
    ADC0->SC1[0] = ADC_SC1_ADCH(ch);

    while(((ADC0->SC1[0]) & ADC_SC1_COCO_MASK) == 0) ;

    return ADC0->R[0];
}

uint16_t adc0_getMeasurement(uint8_t ch)
{
    /*
     * ADC data width is 16 bit: to convert from ADC samples to voltage in mV,
     * first multiply by 3300 and then divide the result by 65536 by right
     * shifting it of 16 positions.
     * With respect to using floats, maximum error is -1mV.
     */
    uint32_t sample = adc0_getRawSample(ch) * 3300;
    return ((uint16_t) (sample >> 16));
}
