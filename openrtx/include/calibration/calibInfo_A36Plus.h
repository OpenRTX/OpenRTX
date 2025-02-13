/***************************************************************************
 *   Copyright (C) 2020 - 2023 by Federico Amedeo Izzo IU2NUO,             *
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

#ifndef CALIBINFO_A36PLUS_H
#define CALIBINFO_A36PLUS_H

#include <datatypes.h>
#include <stdint.h>

/**
 * Data types defining the structure of calibration data stored in external
 * flash memory of A36plus devices.
 */

/**
 * \brief Calibration data for A36plus.
 */
typedef struct
{
   // To be determined
}
A36plusCalib_t;

typedef struct PowerCalibration {
    uint8_t power_below_130mhz;
    uint8_t power_455_470mhz;
    uint8_t power_420_455mhz;
    uint8_t power_300_420mhz;
    uint8_t power_200_300mhz;
    uint8_t power_166_200mhz;
    uint8_t power_145_166mhz;
    uint8_t power_130_145mhz;
    uint8_t unused[56]; // Remaining unused bytes in the 64-byte block
} PowerCalibration;

typedef struct PowerCalibrationTables {
    PowerCalibration high;
    PowerCalibration med;
    PowerCalibration low;
} PowerCalibrationTables;

#endif /* CALIBINFO_A36PLUS_H */

