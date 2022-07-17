/***************************************************************************
 *   Copyright (C) 2020 - 2022 by Silvano Seva IU2KWO                      *
 *                            and Niccolò Izzo IU2KIN                      *
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

#ifndef SKY73210_H
#define SKY73210_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Driver for SKY73210 PLL IC.
 *
 * WARNING: on MD3x0 devices the PLL and DMR chips share the SPI MOSI line,
 * thus particular care has to be put to avoid them stomping reciprocally.
 * This driver does not make any check if a SPI transfer is already in progress,
 * deferring the correct bus management to higher level modules. However,
 * a function returning true if the bus is currently in use by this driver
 * is provided.
 */

/**
 * Initialise the PLL.
 */
void SKY73210_init();

/**
 * Terminate PLL driver, bringing GPIOs back to reset state.
 */
void SKY73210_terminate();

/**
 * Change VCO frequency.
 * @param freq: new VCO frequency, in Hz.
 * @param clkDiv: reference clock division factor.
 */
void SKY73210_setFrequency(float freq, uint8_t clkDiv);

/**
 * Check if PLL is locked.
 * @return true if PLL is locked.
 */
bool SKY73210_isPllLocked();

/**
 * Check if the SPI bus in common between PLL and DMR chips is in use by this
 * driver.
 * @return true if this driver is using the SPI bus.
 */
bool SKY73210_spiInUse();

#ifdef __cplusplus
}
#endif

#endif /* SKY73210_H */
