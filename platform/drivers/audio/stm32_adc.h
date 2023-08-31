/***************************************************************************
 *   Copyright (C) 2023 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
 *                         Frederik Saraci IU2NRO                          *
 *                         Silvano Seva IU2KWO                             *
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

#ifndef STM32_ADC_H
#define STM32_ADC_H

#include <stdbool.h>
#include <stdint.h>
#include <stm32f4xx.h>
#include <interfaces/audio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Driver for STM32F4xx ADC peripheral used as audio input stream device.
 * Output data format is signed 16-bit, hardware outputs positive-only 12-bit
 * samples.
 *
 * This driver has three instances:
 *
 * - instance 0: ADC1
 * - instance 1: ADC2
 * - instance 2: ADC3
 *
 * The configuration parameter for each instance is the ADC input number.
 */


enum Stm32AdcInstance
{
    STM32_ADC_ADC1 = 0,
    STM32_ADC_ADC2,
    STM32_ADC_ADC3,
};


extern const struct audioDriver stm32_adc_audio_driver;


/**
 * Initialize the driver and the peripherals.
 */
void stm32adc_init(const uint8_t instance);

/**
 * Shutdown the driver and the peripherals.
 */
void stm32adc_terminate();

#ifdef __cplusplus
}
#endif

#endif /* STM32_DAC_H */
