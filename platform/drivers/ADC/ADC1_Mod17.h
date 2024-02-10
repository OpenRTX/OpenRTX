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

#ifndef ADC1_H
#define ADC1_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Driver for ADC1, used on Module 17 to sample input voltage
 *
 * NOTE: values inside the enum are the channel numbers of STM32 ADC1 peripheral.
 */

enum adcCh
{
    ADC_HWVER_CH = 3,
    ADC_HMI_HWVER_CH = 13,
};

/**
 * Initialise ADC1.
 */
void adc1_init();

/**
 * Turn off ADC1.
 */
void adc1_terminate();

/**
 * Get current measurement of a given channel returning the raw ADC value.
 *
 * NOTE: the mapping provided in enum adcCh DOES NOT correspond to the physical
 * ADC channel mapping!
 *
 * @param ch: channel number.
 * @return current value of the specified channel, in ADC counts.
 */
uint16_t adc1_getRawSample(uint8_t ch);

/**
 * Get current measurement of a given channel.
 *
 * NOTE: the mapping provided in enum adcCh DOES NOT correspond to the physical
 * ADC channel mapping!
 *
 * @param ch: channel number.
 * @return current value of the specified channel in mV.
 */
uint16_t adc1_getMeasurement(uint8_t ch);

#ifdef __cplusplus
}
#endif

#endif /* ADC1_H */
