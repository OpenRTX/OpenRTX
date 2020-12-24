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
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#include <interfaces/gpio.h>
#include "hwconfig.h"
#include "ADC1_MDx.h"

/*
 * Ringbuffer of samples to allow for value smoothing through averaging. This
 * buffer is structured as follows:
 *
 * | vbat | rssi | vox | vol | vbat | rssi | vox | vol | ...
 *
 * thus it contains four samples of the four channels. Then, DMA is configured
 * in circular mode with a rollover after 16 transfers, effectively managing
 * this buffer as a ringbuffer for the measurements of the four channels.
 * Then, in adc1_getMeasurement(), the average over the values contained in the
 * ring buffer is performed and returned as the current channel value.
 */
uint16_t sampleRingBuf[16];

void adc1_init()
{
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;
    __DSB();

    /*
     * Configure GPIOs to analog input mode:
     * - PA0: volume potentiometer level
     * - PA1: battery voltage
     * - PA3: vox level
     * - PB0: RSSI level
     */
    gpio_setMode(AIN_VBAT,   INPUT_ANALOG);
    #if defined(PLATFORM_MD380) || defined(PLATFORM_MD390)
    gpio_setMode(AIN_VOLUME, INPUT_ANALOG);
    gpio_setMode(AIN_MIC,    INPUT_ANALOG);
    gpio_setMode(AIN_RSSI,   INPUT_ANALOG);
    #endif

    /*
     * ADC clock is APB2 frequency divided by 8, giving 10.5MHz.
     * We set the sample time of each channel to 480 ADC cycles and we have to
     * scan four channels: given that a conversion takes 12 cycles, we have a
     * total conversion time of ~187us.
     */
    ADC->CCR |= ADC_CCR_ADCPRE;
    ADC1->SMPR2 = ADC_SMPR2_SMP0
                | ADC_SMPR2_SMP1
                | ADC_SMPR2_SMP3
                | ADC_SMPR2_SMP8;

    /*
     * No overrun interrupt, 12-bit resolution, no analog watchdog, no
     * discontinuous mode, enable scan mode, no end of conversion interrupts,
     * enable continuous conversion (free-running).
     */
    ADC1->CR1 |= ADC_CR1_SCAN;
    ADC1->CR2 |= ADC_CR2_DMA
              | ADC_CR2_DDS
              | ADC_CR2_CONT
              | ADC_CR2_ADON;

    /* Scan sequence config. */
    #if defined(PLATFORM_MD380) || defined(PLATFORM_MD390)
    ADC1->SQR1 = 3 << 20;    /* Four channels to be converted          */
    ADC1->SQR3 |= (1 << 0)   /* CH1, battery voltage on PA1            */
               |  (8 << 5)   /* CH8, RSSI value on PB0                 */
               |  (3 << 10)  /* CH3, vox level on PA3                  */
               |  (0 << 15); /* CH0, volume potentiometer level on PA0 */
    #else
    ADC1->SQR1 = 0;          /* Convert one channel                    */
    ADC1->SQR3 |= (1 << 0);  /* CH1, battery voltage on PA1            */
    #endif

    /* DMA2 Stream 0 configuration:
     * - channel 0: ADC1
     * - low priority
     * - half-word transfer, both memory and peripheral
     * - increment memory
     * - circular mode
     * - peripheral-to-memory transfer
     * - no interrupts
     */
    DMA2_Stream0->PAR = ((uint32_t) &(ADC1->DR));
    DMA2_Stream0->M0AR = ((uint32_t) &sampleRingBuf);
    DMA2_Stream0->NDTR = 16;
    DMA2_Stream0->CR = DMA_SxCR_MSIZE_0     /* Memory size: 16 bit     */
                     | DMA_SxCR_PSIZE_0     /* Peripheral size: 16 bit */
                     | DMA_SxCR_PL_0        /* Medium priority         */
                     | DMA_SxCR_MINC        /* Increment memory        */
                     | DMA_SxCR_CIRC        /* Circular mode           */
                     | DMA_SxCR_EN;

    /* Finally, start conversion */
    ADC1->CR2 |= ADC_CR2_SWSTART;
}

void adc1_terminate()
{
    DMA2_Stream0->CR &= ~DMA_SxCR_EN;
    ADC1->CR2 &= ~ADC_CR2_ADON;
    RCC->APB2ENR &= ~RCC_APB2ENR_ADC1EN;
    __DSB();
}

float adc1_getMeasurement(uint8_t ch)
{
    if(ch > 3) return 0.0f;

    /* Return the average over the ring buffer */
    float value = 0.0f;
    for(uint8_t i = 0; i < 16; i += 4)
    {
        value += ((float) sampleRingBuf[i + ch]);
    }
    value /= 4.0f;

    /* Return average value converted to mV */
    return (value * 3300.0f)/4096.0f;
}
