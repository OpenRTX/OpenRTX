/***************************************************************************
 *   Copyright (C) 2020 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
 *                         Frederik Saraci IU2NRO                          *
 *                         Silvano Seva IU2KWO                             *
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

#ifndef AT24Cx_H
#define AT24Cx_H

#include <stdint.h>
#include <sys/types.h>

/**
 * Driver for ATMEL AT24Cx family of I2C EEPROM devices, used as external non
 * volatile memory on various radios to store global settings and contact data.
 */

/**
 * Initialise driver for external EEPROM.
 */
void AT24Cx_init();

/**
 * Terminate driver for external EEPROM.
 */
void AT24Cx_terminate();

/**
 * Read data from EEPROM memory.
 *
 * @param addr: start address for read operation.
 * @param buf: pointer to a buffer where data is written to.
 * @param len: number of bytes to read.
 */
void AT24Cx_readData(uint32_t addr, void* buf, size_t len);

#endif /* AT24Cx_H */
