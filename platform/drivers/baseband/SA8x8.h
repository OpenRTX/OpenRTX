/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef SA8x8_H
#define SA8x8_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Setup the GPIOs and UART to control the SA8x8 module and initalize it.
 *
 * @return 0 on success, an error code otherwise.
 */
int sa8x8_init();

/**
 * Get the SA8x8 module type.
 *
 * @return a string describing the module type.
 */
const char *sa8x8_getModel();

/**
 * Get the SA8x8 firmware version.
 *
 * @return firmware version string.
 */
const char *sa8x8_getFwVersion();

/**
 * Enable high-speed mode on the SA8x8 serial port, changing the baud rate to
 * 115200 bit per second.
 *
 * @return 0 on success, an error code otherwise.
 */
int sa8x8_enableHSMode();

/**
 * Set the transmission power.
 *
 * @param power: transmission power in mW.
 */
void sa8x8_setTxPower(const uint32_t power);

/**
 * Enable or disable the speaker power amplifier.
 *
 * @param value: boolean state of the speaker power amplifier.
 */
void sa8x8_setAudio(bool value);

/**
 * Write a register of the AT1846S radio IC contained in the SA8x8 module.
 *
 * @param reg: register number.
 * @param value: value to be written.
 */
void sa8x8_writeAT1846Sreg(uint8_t reg, uint16_t value);

/**
 * Read a register of the AT1846S radio IC contained in the SA8x8 module.
 *
 * @param reg: register number.
 * @return register value.
 */
uint16_t sa8x8_readAT1846Sreg(uint8_t reg);

#ifdef __cplusplus
}
#endif

#endif /* SA8x8_H */
