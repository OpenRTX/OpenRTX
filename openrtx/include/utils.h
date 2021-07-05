/***************************************************************************
 *   Copyright (C) 2020 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN,                            *
 *                         Frederik Saraci IU2NRO,                         *
 *                         Silvano Seva IU2KWO,                            *
 *                         Federico Terraneo                               *
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

#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Calculate the ccitt crc16 on a string of bytes
 * \param message string of bytes
 * \param length message length
 * \return the crc16
 */
uint16_t crc16(const void *message, size_t length);

/**
 * Dump a memory area in this format
 * 0x00000000 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 | ................
 * \param start pointer to beginning of memory block to dump
 * \param len length of memory block to dump
 */
void memDump(const void *start, int len);

#ifdef __cplusplus
}
#endif

#endif  /* UTIL_H */
