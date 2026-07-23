/*
 * SPDX-FileCopyrightText: Copyright 2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Driver for the HR_C7000 on-chip ADC (used on the Ailunce HD2 and other
 * HR_C7000 targets), implementing the standard peripherals/adc.h interface.
 * Single-shot 10-bit conversions, busy-polled.
 */

#ifndef ADC_HRC7000_H
#define ADC_HRC7000_H

#include "peripherals/adc.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Define an instance of the HR_C7000 ADC device driver.  The chip has a single
 * ADC block, so no peripheral base is needed (priv is unused).
 *
 * @param name: instance name.
 * @param mutx: pointer to mutex for concurrent access, can be NULL.
 * @param res:  conversion factor from counts to uV (see ADC_COUNTS_TO_UV).
 */
#define ADC_HRC7000_DEVICE_DEFINE(name, mutx, res)             \
    extern uint16_t adcHrc7000_sample(const struct Adc *adc,   \
                                      const uint32_t channel); \
    const struct Adc name = {                                  \
        .sample = &adcHrc7000_sample,                          \
        .priv = NULL,                                          \
        .mutex = mutx,                                         \
        .countsTouV = res,                                     \
    };

/**
 * Initialise the HR_C7000 ADC (the vendor reset-release pulse the conversion
 * FSM needs; without it DATA reads 0) and its mutex, if any.
 *
 * @param adc: pointer to ADC device handle.
 * @return zero on success.
 */
int adcHrc7000_init(const struct Adc *adc);

/**
 * Shut down the HR_C7000 ADC.
 *
 * @param adc: pointer to ADC device handle.
 */
void adcHrc7000_terminate(const struct Adc *adc);

#ifdef __cplusplus
}
#endif

#endif /* ADC_HRC7000_H */
