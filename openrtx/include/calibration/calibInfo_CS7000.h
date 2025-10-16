/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CALIBINFO_CS7000_H
#define CALIBINFO_CS7000_H

#include "core/datatypes.h"
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

/**
 * \brief RSSI calibration data.
 */
struct rssiParams
{
    float    slope;
    float    offset;
    uint32_t rxFreq;
};

#endif /* CALIBINFO_CS7000_H */
