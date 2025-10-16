/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef STM32_ADC_H
#define STM32_ADC_H

#include <stdbool.h>
#include <stdint.h>
#include "interfaces/audio.h"

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
