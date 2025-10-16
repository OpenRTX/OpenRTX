/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef Cx000_DAC_H
#define Cx000_DAC_H

#include <stdbool.h>
#include <stdint.h>
#include "interfaces/audio.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Driver to use the HR_Cx000 internal DAC as audio output stream device.
 * Input data format is signed 16-bit and only a single instance of this driver
 * is allowed.
 */

extern const struct audioDriver Cx000_dac_audio_driver;

/**
 * Start generation of a "beep" tone from DAC output.
 *
 * @param freq: tone frequency in Hz.
 * @return zero on success, a negative error code otherwise.
 */
int Cx000dac_startBeep(const uint16_t freq);

/**
 * Stop an ongoing "beep" tone.
 */
void Cx000dac_stopBeep();

#ifdef __cplusplus
}   // extern "C"

/**
 * Initialize the driver.
 */
void Cx000dac_init(HR_C6000 *device);

/**
 * Shutdown the driver.
 */
void Cx000dac_terminate();

/**
 * Driver task function, to be called at least once every 4ms to ensure a
 * proper operation.
 */
void Cx000dac_task();

#endif

#endif /* Cx000_DAC_H */
