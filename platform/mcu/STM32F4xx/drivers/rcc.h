/***************************************************************************
 *   Copyright (C) 2024 by Federico Amedeo Izzo IU2NUO,                    *
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

#ifndef RCC_H
#define RCC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Enumeration type for STM32 internal busses.
 */
enum PeriphBus
{
    PERIPH_BUS_AHB  = 0,
    PERIPH_BUS_APB1 = 1,
    PERIPH_BUS_APB2 = 2,

    PERIPH_BUS_NUM
};

/**
 * Get the clock frequency of a given peripheral bus.
 *
 * @param bus: bus identifier.
 * @return bus clock frequency in Hz or zero in case of errors.
 */
uint32_t rcc_getBusClock(const uint8_t bus);

#ifdef __cplusplus
}
#endif

#endif /* RCC_H */
