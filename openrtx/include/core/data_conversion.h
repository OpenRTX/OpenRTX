/***************************************************************************
 *   Copyright (C) 2022 by Federico Amedeo Izzo IU2NUO,                    *
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

#ifndef DATA_CONVERSION_H
#define DATA_CONVERSION_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * In-place conversion of data elements from int16_t to unsigned 16 bit values
 * ranging from 0 to 4095.
 *
 * @param buffer: data buffer.
 * @param length: buffer length, in elements.
 */
void S16toU12(int16_t *buffer, const size_t length);

/**
 * In-place conversion of data elements from int16_t to unsigned 8 bit values
 * ranging from 0 to 255.
 *
 * @param buffer: data buffer.
 * @param length: buffer length, in elements.
 */
void S16toU8(int16_t *buffer, const size_t length);

#ifdef __cplusplus
}
#endif

#endif /* DATA_CONVERSION_H */
