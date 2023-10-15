/***************************************************************************
 *   Copyright (C) 2023 by Silvano Seva IU2KWO                             *
 *                     and Niccolò Izzo IU2KIN                             *
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
 * @param power: transmission power in Watt.
 */
void sa8x8_setTxPower(const float power);

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
