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

#ifndef HRC6000_H
#define HRC6000_H

#include <stdint.h>
#include <stdbool.h>

#include <calibInfo_GDx.h>

/**
 * Initialise the HR_C6000 driver.
 */
void C6000_init();

/**
 * Terminate the HR_C6000 driver.
 */
void C6000_terminate();

/**
 * 
 */
void C6000_setModOffset(uint16_t offset);

/**
 * 
 */
void C6000_setMod1Amplitude(uint8_t amplitude);

/**
 * 
 */
void C6000_setMod2Bias(uint8_t bias);

/**
 * 
 */
void C6000_setDacRange(uint8_t value);

/**
 * Check if SPI common to HR_C6000 and PLL is in use by this driver.
 * @return true if SPI lines are being used by this driver.
 */
bool C6000_spiInUse();

#endif /* HRC6000_H */
