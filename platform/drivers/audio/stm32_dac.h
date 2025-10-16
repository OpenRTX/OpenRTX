/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef STM32_DAC_H
#define STM32_DAC_H

#include <stdbool.h>
#include <stdint.h>
#include "interfaces/audio.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Driver for STM32F4xx DAC peripheral used as audio output stream device.
 * Input data format is signed 16-bit, internally converted to unsigned 12-bit
 * values for compatibility with the hardware.
 *
 * This driver has two instances:
 *
 * - instance 0: DAC_CH1, DMA1_Stream5, TIM6,
 * - instance 1: DAC_CH2, DMA1_Stream6, TIM7
 *
 * The possible configuration for each channel is the idle level for the DAC
 * output, ranging from 0 to 4096. Idle level is passed by value directly in the
 * config field.
 */


enum Stm32DacInstance
{
    STM32_DAC_CH1 = 0,
    STM32_DAC_CH2,
};


extern const struct audioDriver stm32_dac_audio_driver;


/**
 * Initialize the driver and the peripherals.
 *
 * @param instance: DAC instance number.
 * @param idleLevel: DAC output level when idle.
 */
void stm32dac_init(const uint8_t instance, const uint16_t idleLevel);

/**
 * Shutdown the driver and the peripherals.
 */
void stm32dac_terminate();


#ifdef __cplusplus
}
#endif

#endif /* STM32_DAC_H */
