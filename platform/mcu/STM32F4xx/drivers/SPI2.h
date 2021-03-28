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

#ifndef SPI2_H
#define SPI2_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/**
 * Initialise SPI2 peripheral with a bus clock frequency of ~1.3MHz.
 * NOTE: this driver does not configure the SPI GPIOs, which have to be put in
 * alternate mode by application code.
 */
void spi2_init();

/**
 * Shut down SPI2 peripheral.
 * NOTE: is left to application code to change the operating mode of the SPI
 * GPIOs
 */
void spi2_terminate();

/**
 * Exchange one byte over the SPI bus.
 * @param val: transmitted byte.
 * @return incoming byte from slave device.
 */
uint8_t spi2_sendRecv(const uint8_t val);

/**
 * Acquire exclusive ownership on the SPI peripheral by locking an internal
 * mutex. This function is nonblocking and returs true if mutex has been
 * successfully locked by the caller.
 * @return true if device has been locked.
 */
bool spi2_lockDevice();

/**
 * Acquire exclusive ownership on the SPI peripheral by locking an internal
 * mutex. In case mutex is already locked, this function blocks the execution
 * flow until it becomes free again.
 */
void spi2_lockDeviceBlocking();

/**
 * Release exclusive ownership on the SPI peripheral.
 */
void spi2_releaseDevice();

#endif /* SPI2_H */
