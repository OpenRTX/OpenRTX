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

#ifndef HRC5000_WRAPPER_H
#define HRC5000_WRAPPER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Driver for HR_C5000 "baseband" chip.
 *
 * WARNING: on MD3x0 devices the PLL and DMR chips share the SPI MOSI line,
 * thus particular care has to be put to avoid them stomping reciprocally.
 * This driver does not make any check if a SPI transfer is already in progress,
 * deferring the correct bus management to higher level modules. However,
 * a function returning true if the bus is currently in use by this driver is
 * provided.
 */

/**
 * Initialise the HR_C5000 driver.
 */
void C5000_init();

/**
 * Terminate the HR_C5000 driver.
 */
void C5000_terminate();

/**
 * Set value for two-point modulation offset adjustment. This value usually is
 * stored in radio calibration data.
 * @param offset: value for modulation offset adjustment.
 */
void C5000_setModOffset(uint8_t offset);

/**
 * Set values for two-point modulation amplitude adjustment. These values
 * usually are stored in radio calibration data.
 * @param iMag: value for modulation offset adjustment.
 */
void C5000_setModAmplitude(uint8_t iAmp, uint8_t qAmp);

/**
 * Set value for FM-mode modulation factor, a value dependent on bandwidth.
 * @param mf: value for FM modulation factor.
 */
void C5000_setModFactor(uint8_t mf);

/**
 * Configure chipset for DMR operation.
 */
void C5000_dmrMode();

/**
 * Configure chipset for analog FM operation.
 */
void C5000_fmMode();

/**
 * Start analog FM transmission.
 */
void C5000_startAnalogTx();

/**
 * Stop analog FM transmission.
 */
void C5000_stopAnalogTx();

#ifdef __cplusplus
}
#endif

#endif // HRC5000_WRAPPER_H
