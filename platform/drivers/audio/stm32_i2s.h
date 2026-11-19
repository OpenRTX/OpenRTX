/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef STM32_I2S_H
#define STM32_I2S_H

#include <stdint.h>
#include "interfaces/audio.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Driver for the STM32F4 I2S3 peripheral as an audio input stream device, in
 * slave receive mode: the codec drives CK and WS. Philips standard, 16-bit
 * data in 32-bit channels, 8 kHz.
 *
 * The configuration parameter is the channel the stream carries: 0 for the
 * left one, 1 for the right one.
 *
 * Pins come from the target's pinmap: I2S3_WS, I2S3_CK and I2S3_SD. DMA1
 * Stream 0, channel 0.
 */

/**
 * Initialise the I2S3 peripheral, its pins and its DMA stream.
 */
void stm32i2s_init();

/**
 * Shut down the I2S3 peripheral.
 */
void stm32i2s_terminate();

/**
 * I2S channel to take, passed as the driver's configuration.
 */
enum stm32I2sChannel {
    STM32_I2S_LEFT = 0,
    STM32_I2S_RIGHT = 1,
};

extern const struct audioDriver stm32_i2s_audio_driver;

#ifdef __cplusplus
}
#endif

#endif /* STM32_I2S_H */
