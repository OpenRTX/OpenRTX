/***************************************************************************
 *   Copyright (C) 2021 - 2025 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN                      *
 *                                Frederik Saraci IU2NRO                   *
 *                                Silvano Seva IU2KWO                      *
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

#ifndef GPS_ZEPHYR_H
#define GPS_ZEPHYR_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialise the GPS driver.
 * This function does not turn on the GPS module.
 */
void gpsZephyr_init();

/**
 * Terminate the GPS driver.
 */
void gpsZephyr_terminate();

/**
 * Retrieve a NMEA sentence from the GPS.
 * If the sentence is longer than the maximum size of the destination buffer,
 * the characters not written in the destination are lost.
 *
 * @param priv: unused parameter, for function signature compatibility.
 * @param buf: pointer to NMEA sentence destination buffer.
 * @param maxLen: maximum acceptable size for the destination buffer.
 * @return the length of the extracted sentence or -1 if the sentence is longer
 * than the maximum allowed size. If the ring buffer is empty, zero is returned.
 */
int gpsZephyr_getNmeaSentence(void *priv, char *buf, const size_t maxLength);

#ifdef __cplusplus
}
#endif

#endif /* GPS_STM32_H */
