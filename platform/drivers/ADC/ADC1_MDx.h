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

#ifndef ADC1_H
#define ADC1_H

#include <stdint.h>

/**
 * Driver for ADC1, used to continuously sample the following channels:
 * - ADC1_CH0 (PA0): output value of the volume potentiometer;
 * - ADC1_CH1 (PA1): battery voltage through 1:3 resistor divider;
 * - ADC1_CH3 (PA3): vox level;
 * - ADC1_CH8 (PB0): RSSI level;
 */

/**
 * Initialise and start ADC1 and DMA2 Stream 0.
 *
 * ADC is configured in free-running mode with 1:8 prescaler and a sample time
 * for each channel of 480 cycles. This gives a sampling frequency, for each
 * channel, of ~5.3kHz.
 *
 * DMA2 Stream 0 is used to transfer data from ADC1 data register to an internal
 * buffer, from which is fetched by application code using adc1_getMeasurement().
 */
void adc1_init();

/**
 * Turn off ADC1 (also gating off its clock) and disable DMA2 Stream 0.
 * DMA2 clock is kept active.
 */
void adc1_terminate();

/**
 * Get current measurement of a given channel, mapped as below:
 * - channel 0: battery voltage
 * - channel 1: volume level
 * - channel 2: RSSI level
 * - channel 3: vox level
 *
 * NOTE: the mapping above DOES NOT correspond to the physical ADC channel
 * mapping!
 *
 * @param ch: channel number, between 0 and 3.
 * @return current value of the specified channel in mV.
 */
float adc1_getMeasurement(uint8_t ch);

#endif /* ADC1_H */
