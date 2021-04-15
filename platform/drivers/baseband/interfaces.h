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

#ifndef INTERFACES_H
#define INTERFACES_H

#include <stdint.h>

/*
 * This file provides a standard interface for low-level data exchange with the
 * baseband chipset (HR_C6000, AT1846S, ...).
 * Its aim is to provide a decoupling layer between the chipset drivers, written
 * to be platform-agnostic, and the platform-specific communication busses.
 */

/**
 * HR_C5000 and HR_C6000: initialise "user" SPI interface, the one for chip
 * configuration.
 */
void uSpi_init();

/**
 * HR_C5000 and HR_C6000: transfer one byte over the "user" SPI interface.
 */
uint8_t uSpi_sendRecv(uint8_t val);

#endif  /* INTERFACES_H */
