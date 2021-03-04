/***************************************************************************
 *   Copyright (C) 2020 by Silvano Seva IU2KWO and Niccolò Izzo IU2KIN     *
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
 *   As a special exception, if other files instantiate templates or use   *
 *   macros or inline functions from this file, or you compile this file   *
 *   and link it with other works to produce a work based on this file,    *
 *   this file does not by itself cause the resulting work to be covered   *
 *   by the GNU General Public License. However the source code for this   *
 *   file must still be made available in accordance with the GNU General  *
 *   Public License. This exception does not invalidate any other reasons  *
 *   why a work based on this file might be covered by the GNU General     *
 *   Public License.                                                       *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#ifndef PLL_MD3x0_H
#define PLL_MD3x0_H

#include <stdint.h>
#include <stdbool.h>

/**
 * Driver for PLL in MD3x0 radios (MD380 and MD380), which is SKY73210.
 *
 * WARNING: the PLL and DMR chips share the SPI MOSI line, thus particular care
 * has to be put to avoid them stomping reciprocally. This driver does not make
 * any check if a SPI transfer is already in progress, deferring the correct bus
 * management to higher level modules. However, a function returning true if the
 * bus is currently in use by this driver is provided.
 */

/**
 * Initialise the PLL.
 */
void pll_init();

/**
 * Terminate PLL driver, bringing GPIOs back to reset state.
 */
void pll_terminate();

/**
 * Change VCO frequency.
 * @param freq: new VCO frequency, in Hz.
 * @param clkDiv: reference clock division factor.
 */
void pll_setFrequency(uint32_t freq, uint8_t clkDiv);

/**
 * Check if PLL is locked.
 * @return true if PLL is locked.
 */
bool pll_locked();

/**
 * Check if the SPI bus in common between PLL and DMR chips is in use by this
 * driver.
 * @return true if this driver is using the SPI bus.
 */
bool pll_spiInUse();

#endif /* PLL_H */
