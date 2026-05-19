/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CALIBINFO_MDX_H
#define CALIBINFO_MDX_H

#include "core/datatypes.h"
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
