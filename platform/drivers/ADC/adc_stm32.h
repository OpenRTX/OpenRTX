/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef ADC_STM32_H
#define ADC_STM32_H

#include "peripherals/adc.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Define an instance of an STM32 ADC device driver.
 *
 * @param name: instance name.
 * @param periph: pointer to hardware peripheral.
 * @param mutx: pointer to mutex for concurrent access, can be NULL.
 * @param res: ADC resolution in uV/LSB.
 */
#define ADC_STM32_DEVICE_DEFINE(name, periph, mutx, res)  \
extern uint16_t adcStm32_sample(const struct Adc *adc,    \
                                const uint32_t channel);  \
const struct Adc name =                                   \
{                                                         \
    .sample     = &adcStm32_sample,                       \
    .priv       = periph,                                 \
    .mutex      = mutx,                                   \
    .countsTouV = res                                     \
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
