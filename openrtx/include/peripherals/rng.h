/***************************************************************************
 *   Copyright (C) 2023 by Federico Amedeo Izzo IU2NUO,                    *
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

#ifndef RNG_H
#define RNG_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Driver for Random Number Generator.
 */

/**
 * Initialise the RNG driver.
 */
void rng_init();

/**
 * Shut down the RNG module.
 */
void rng_terminate();

/**
 * Generate a 32 bit random number.
 *
 * @return random number.
 */
uint32_t rng_get();

#ifdef __cplusplus
}
#endif

#endif /* INTERFACES_GPS_H */
