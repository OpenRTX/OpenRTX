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

#ifndef ADC1_H
#define ADC1_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

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
 *
 * NOTE: values inside the enum are the channel numbers of STM32 ADC1 peripheral.
 */

enum adcCh
{
    ADC_VOL_CH   = 0,
    ADC_VBAT_CH  = 1,
    ADC_VOX_CH   = 3,
    ADC_RSSI_CH  = 8,
    ADC_SW1_CH   = 7,
    ADC_SW2_CH   = 6,
    ADC_RSSI2_CH = 9,
    ADC_HTEMP_CH = 15
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
