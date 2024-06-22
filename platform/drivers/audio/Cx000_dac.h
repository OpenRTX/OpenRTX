/***************************************************************************
 *   Copyright (C) 2024 by Silvano Seva IU2KWO                             *
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

#ifndef Cx000_DAC_H
#define Cx000_DAC_H

#include <stdbool.h>
#include <stdint.h>
#include <interfaces/audio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Driver to use the HR_Cx000 internal DAC as audio output stream device.
 * Input data format is signed 16-bit and only a single instance of this driver
 * is allowed.
 */

extern const struct audioDriver Cx000_dac_audio_driver;

#ifdef __cplusplus
}   // extern "C"

/**
 * Initialize the driver.
 */
void Cx000dac_init(HR_C6000 *device);

/**
 * Shutdown the driver.
 */
void Cx000dac_terminate();

/**
 * Driver task function, to be called at least once every 4ms to ensure a
 * proper operation.
 */
void Cx000dac_task();

#endif

#endif /* Cx000_DAC_H */
