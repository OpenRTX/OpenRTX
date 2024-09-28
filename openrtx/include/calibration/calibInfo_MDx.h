/***************************************************************************
 *   Copyright (C) 2020 - 2024 by Federico Amedeo Izzo IU2NUO,             *
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

#ifndef CALIBINFO_MDX_H
#define CALIBINFO_MDX_H

#include <datatypes.h>
#include <stdint.h>

/**
 * Data types defining the structure of calibration data stored in external
 * flash memory of MDx devices.
 *
 * Single band and dual band devices use more or less the same calibration data
 * entries, thus a single data structure have been made. The only difference
 * between the two is that the dual band radios have five calibration points
 * for the VHF band instead of nine.
 */

struct CalData
{
    uint8_t freqAdjustMid;
    freq_t  rxFreq[9];
    freq_t  txFreq[9];
    uint8_t txHighPower[9];
    uint8_t txLowPower[9];
    uint8_t rxSensitivity[9];
    uint8_t sendIrange[9];
    uint8_t sendQrange[9];
    uint8_t analogSendIrange[9];
    uint8_t analogSendQrange[9];
};

/**
 * \brief Calibration data for MD-3x0.
 */
typedef struct CalData md3x0Calib_t;

/**
 * \brief Calibration data for MD-UV3x0.
 */
typedef struct
{
    struct CalData uhfCal;
    struct CalData vhfCal;
}
mduv3x0Calib_t;

#endif /* CALIBINFO_MDX_H */
