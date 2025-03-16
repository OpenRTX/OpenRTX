/***************************************************************************
 *   Copyright (C) 2024 by Silvano Seva IU2KWO                             *
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

#ifndef ADC_STM32_H
#define ADC_STM32_H

#include <peripherals/adc.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Define an instance of an STM32 ADC device driver.
 *
 * @param name: instance name.
 * @param periph: pointer to hardware peripheral.
 * @param mutx: pointer to mutex for concurrent access, can be NULL.
 * @param vref: ADC reference voltage, in uV.
 */
#define ADC_STM32_DEVICE_DEFINE(name, periph, mutx, vref) \
extern uint16_t adcStm32_sample(const struct Adc *adc,    \
                                const uint32_t channel);  \
const struct Adc name =                                   \
{                                                         \
    .sample     = &adcStm32_sample,                       \
    .priv       = periph,                                 \
    .mutex      = mutx,                                   \
    .countsTouV = ADC_COUNTS_TO_UV(vref, 12)              \
};

/**
 * Initialize an STM32 ADC peripheral.
 *
 * @param adc: pointer to ADC device handle.
 * @return zero on success, a negative error code otherwise.
 */
int adcStm32_init(const struct Adc *adc);

/**
 * Shut down an STM32 ADC peripheral.
 *
 * @param adc: pointer to ADC device handle.
 * @return zero on success, a negative error code otherwise.
 */
void adcStm32_terminate(const struct Adc *adc);

#ifdef __cplusplus
}
#endif

#endif /* ADC_STM32_H */
