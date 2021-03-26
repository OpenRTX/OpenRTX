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
 * Driver for ADC1, used on all the MDx devices to continuously sample battery
 * voltage and other values.
 *
 * Channel mapping for MDx platforms:
 *
 *                                +--------+----------+---------+
 *                                | MD-3x0 | MD-UV3x0 | MD-9600 |
 * +-----+------+-----------------+--------+----------+---------+
 * | PA0 | IN0  | volume level    |   x    |    x     |         |
 * | PA1 | IN1  | supply voltage  |   x    |    x     |    x    |
 * | PA3 | IN3  | mic level (VOX) |   x    |    x     |    x    |
 * | PA6 | IN6  | mic SW2 line    |        |          |    x    |
 * | PA7 | IN7  | mic SW1 line    |        |          |    x    |
 * | PB0 | IN8  |    RSSI         |   x    |          |    x    |
 * | PB1 | IN9  |                 |        |          |    x    |
 * | PC5 | IN15 | heatsink temp.  |        |          |    x    |
 * +-----+------+-----------------+--------+----------+---------+
 */

enum adcCh
{
    ADC_VOL_CH   = 0,
    ADC_VBAT_CH  = 1,
    ADC_VOX_CH   = 2,
    ADC_RSSI_CH  = 3,
    ADC_SW1_CH   = 4,
    ADC_SW2_CH   = 5,
    ADC_RSSI2_CH = 6,
    ADC_HTEMP_CH = 7
};

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
 * Get current measurement of a given channel.
 *
 * NOTE: the mapping provided in enum adcCh DOES NOT correspond to the physical
 * ADC channel mapping!
 *
 * @param ch: channel number.
 * @return current value of the specified channel in mV.
 */
float adc1_getMeasurement(uint8_t ch);

#endif /* ADC1_H */
