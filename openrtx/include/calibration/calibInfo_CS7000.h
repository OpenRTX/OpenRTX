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

#ifndef CALIBINFO_CS7000_H
#define CALIBINFO_CS7000_H

#include <datatypes.h>
#include <stdint.h>


/**
 * \brief Calibration data for Connect Systems CS7000.
 */
struct CS7000Calib
{
    uint32_t txCalFreq[8];       // 0x000
    uint32_t rxCalFreq[8];       // 0x024
    uint8_t  rxSensitivity[8];   // 0x044
    uint8_t  txHighPwr[8];       // 0x06C
    uint8_t  txMiddlePwr[8];     // 0x074
    uint8_t  mskFreqOffset[8];   // 0x0B4
    uint8_t  txDigitalPathI[8];  // 0x0BC
    uint8_t  txDigitalPathQ[8];  // 0x0C4
    uint8_t  txAnalogPathI[8];   // 0x0CC
    uint8_t  txAnalogPathQ[8];   // 0x0D4
    uint8_t  errorRate[8];       // 0x0DC
};

#endif /* CALIBINFO_CS7000_H */
