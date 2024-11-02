/***************************************************************************
 *   Copyright (C) 2018 by Terraneo Federico                               *
 *                 2024 by Terraneo Federico and Silvano Seva IU2KWO       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
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

#ifndef PLL_H
#define PLL_H

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
    PERIPH_BUS_APB4 = 3,

    PERIPH_BUS_NUM
};

/**
 * Configure and start the PLL.
 */
void startPll();

/**
 * Get the clock frequency of a given peripheral bus.
 *
 * @param bus: bus identifier.
 * @return bus clock frequency in Hz or zero in case of errors.
 */
uint32_t getBusClock(const uint8_t bus);

#ifdef __cplusplus
}
#endif

#endif //PLL_H
