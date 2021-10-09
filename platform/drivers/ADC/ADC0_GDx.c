/***************************************************************************
 *   Copyright (C) 2020 by Silvano Seva IU2KWO and Niccolò Izzo IU2KIN     *
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
 *   As a special exception, if other files instantiate templates or use   *
 *   macros or inline functions from this file, or you compile this file   *
 *   and link it with other works to produce a work based on this file,    *
 *   this file does not by itself cause the resulting work to be covered   *
 *   by the GNU General Public License. However the source code for this   *
 *   file must still be made available in accordance with the GNU General  *
 *   Public License. This exception does not invalidate any other reasons  *
 *   why a work based on this file might be covered by the GNU General     *
 *   Public License.                                                       *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#include <hwconfig.h>
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
