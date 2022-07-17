/***************************************************************************
 *   Copyright (C) 2020 - 2022 by Federico Amedeo Izzo IU2NUO,             *
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

#include <stdint.h>

#include "core/datatypes.h"

/**
 * Data types defining the structure of calibration data stored in external
 * flash memory of MDx devices.
 */

/**
 * \brief Calibration data for MD-3x0.
 */
typedef struct
{
    uint8_t vox1;
    uint8_t vox10;
    uint8_t rxLowVoltage;
    uint8_t rxFullVoltage;
    uint8_t rssi1;
    uint8_t rssi4;
    uint8_t analogMic;
    uint8_t digitalMic;
    uint8_t freqAdjustHigh;
    uint8_t freqAdjustMid;
    uint8_t freqAdjustLow;
    freq_t  rxFreq[9];
    freq_t  txFreq[9];
    uint8_t txHighPower[9];
    uint8_t txLowPower[9];
    uint8_t rxSensitivity[9];
    uint8_t openSql9[9];
    uint8_t closeSql9[9];
    uint8_t openSql1[9];
    uint8_t closeSql1[9];
    uint8_t maxVolume[9];
    uint8_t ctcss67Hz[9];
    uint8_t ctcss151Hz[9];
    uint8_t ctcss254Hz[9];
    uint8_t dcsMod2[9];
    uint8_t dcsMod1[9];
    uint8_t mod1Partial[9];
    uint8_t analogVoiceAdjust[9];
    uint8_t lockVoltagePartial[9];
    uint8_t sendIpartial[9];
    uint8_t sendQpartial[9];
    uint8_t sendIrange[9];
    uint8_t sendQrange[9];
    uint8_t rxIpartial[9];
    uint8_t rxQpartial[9];
    uint8_t analogSendIrange[9];
    uint8_t analogSendQrange[9];
}
md3x0Calib_t;

typedef struct
{
    uint8_t freqAdjustMid;
    freq_t  rxFreq[5];
    freq_t  txFreq[5];
    uint8_t txHighPower[5];
    uint8_t txMidPower[5];
    uint8_t txLowPower[5];
    uint8_t rxSensitivity[5];
    uint8_t openSql9[5];
    uint8_t closeSql9[5];
    uint8_t openSql1[5];
    uint8_t closeSql1[5];
    uint8_t ctcss67Hz[5];
    uint8_t ctcss151Hz[5];
    uint8_t ctcss254Hz[5];
    uint8_t dcsMod1[5];
    uint8_t sendIrange[5];
    uint8_t sendQrange[5];
    uint8_t analogSendIrange[5];
    uint8_t analogSendQrange[5];
}
VhfCalib_t;

typedef struct
{
    uint8_t freqAdjustMid;
    freq_t  rxFreq[9];
    freq_t  txFreq[9];
    uint8_t txHighPower[9];
    uint8_t txMidPower[9];
    uint8_t txLowPower[9];
    uint8_t rxSensitivity[9];
    uint8_t openSql9[9];
    uint8_t closeSql9[9];
    uint8_t openSql1[9];
    uint8_t closeSql1[9];
    uint8_t ctcss67Hz[9];
    uint8_t ctcss151Hz[9];
    uint8_t ctcss254Hz[9];
    uint8_t dcsMod1[9];
    uint8_t sendIrange[9];
    uint8_t sendQrange[9];
    uint8_t analogSendIrange[9];
    uint8_t analogSendQrange[9];
}
UhfCalib_t;

/**
 * \brief Calibration data for MD-UV3x0.
 */
typedef struct
{
    uint8_t vox1;
    uint8_t vox10;
    uint8_t rxLowVoltage;
    uint8_t rxFullVoltage;
    uint8_t rssi1;
    uint8_t rssi4;
    VhfCalib_t vhfCal;
    UhfCalib_t uhfCal;
}
mduv3x0Calib_t;

#endif /* CALIBINFO_MDX_H */
