/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CALIBINFO_C62_H
#define CALIBINFO_C62_H

#include "core/datatypes.h"
#include <stdbool.h>
#include <stdint.h>

/**
 * Maximum number of TX power calibration points per band.
 */
#define C62_CAL_POINTS 10

/**
 * TX power calibration of one band, sorted by frequency: the APC PWM duty
 * cycle, in percent, of the stock low and high power settings at each point.
 */
struct C62BandCalib {
    freq_t freq[C62_CAL_POINTS];
    uint8_t lowPwr[C62_CAL_POINTS];
    uint8_t highPwr[C62_CAL_POINTS];
    uint8_t points;
};

/**
 * C62 calibration data.
 */
struct C62Calib {
    struct C62BandCalib vhf;
    struct C62BandCalib uhf;
    bool fromFlash; ///< False when the stock default table is in use
};

#endif /* CALIBINFO_C62_H */
