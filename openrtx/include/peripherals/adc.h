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

#ifndef ADC_H
#define ADC_H

#include <pthread.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


struct Adc;

/**
 * Sample one ADC channel returning the raw converted value.
 *
 * @param adc: pointer to ADC device handle.
 * @param channel: channel number.
 * @return channel raw value, zero if channel is out of range.
 */
typedef uint16_t (*adc_sample_impl)(const struct Adc *adc, const uint32_t channel);

/**
 * ADC device handle.
 */
struct Adc
{
    const adc_sample_impl sample;      ///< Device-specific implementation of the ADC sample function
    const void            *priv;       ///< Pointer to device driver private data
    const pthread_mutex_t *mutex;      ///< Pointer to mutex, can be NULL
    const uint32_t        countsTouV;  ///< Conversion factor from counts to uV
};

/**
 * Compute conversion factor from ADC counts to uV expressed in Q16.16 format.
 *
 * @param vref: ADC reference voltage, in uV.
 * @param resolution: ADC resolution, in bits.
 * @return conversion factor.
 */
#define ADC_COUNTS_TO_UV(vref, resolution) (vref << (16 - resolution))

/**
 * Sample one ADC channel returning the raw converted value.
 *
 * @param adc: pointer to ADC device handle.
 * @param channel: channel number.
 * @return channel raw value, zero if channel is out of range.
 */
static inline uint16_t adc_getRawSample(const struct Adc *adc, const uint32_t channel)
{
    if(adc->mutex != NULL)
        pthread_mutex_lock((pthread_mutex_t *) adc->mutex);

    uint16_t sample = adc->sample(adc, channel);

    if(adc->mutex != NULL)
        pthread_mutex_unlock((pthread_mutex_t *) adc->mutex);

    return sample;
}

/**
 * Sample one ADC channel returning the voltage, in uV.
 *
 * @param adc: pointer to ADC device handle.
 * @param channel: channel number.
 * @return channel voltage, zero if channel is out of range.
 */
static inline uint32_t adc_getVoltage(const struct Adc *adc, const uint32_t channel)
{
    uint64_t value = adc_getRawSample(adc, channel);
    return (value * adc->countsTouV) >> 16;
}

#ifdef __cplusplus
}
#endif

#endif /* ADC_H */
