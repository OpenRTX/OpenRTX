/***************************************************************************
 *   Copyright (C) 2021 by Federico Amedeo Izzo IU2NUO,                    *
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

#ifndef MCUFLASH_H
#define MCUFLASH_H

#include <stdint.h>
#include <stdbool.h>

/**
 * Driver for MCU's internal flash management, allowing for sector erase and data
 * writing.
 */

/**
 * Erase one sector of the MCU flash memory.
 *
 * @param secNum: sector number.
 * @return true for successful erase, false otherwise.
 */
bool mcuFlash_eraseSector(const uint8_t secNum);

/**
 * Write data to the MCU flash memory.
 *
 * @param address: starting address for the write operation.
 * @param data: data to be written.
 * @param len: data length.
 */
void mcuFlash_write(const uint32_t address, const void *data, const size_t len);

#endif /* MCUFLASH_H */