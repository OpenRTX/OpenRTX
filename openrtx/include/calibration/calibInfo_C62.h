/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CALIBINFO_C62_H
#define CALIBINFO_C62_H

#include "core/datatypes.h"
#include <stdint.h>

/**
 * Maximum number of TX power calibration points per band.
 */
#define C62_CAL_POINTS 10

/**
 * TX power calibration point: the APC PWM duty cycle, in percent, of the
 * stock low and high power settings at a frequency. Same layout as the start
 * of a calibration record in the stock firmware's settings journal.
 */
struct C62CalPoint {
    uint32_t freq;
    uint16_t loPwr;
    uint16_t hiPwr;
};

/**
 * TX power calibration of one band, sorted by frequency.
 */
struct C62BandCalib {
    struct C62CalPoint points[C62_CAL_POINTS];
    uint8_t count;
};

/**
 * C62 calibration data.
 */
struct C62Calib {
    struct C62BandCalib vhf;
    struct C62BandCalib uhf;
};

#endif /* CALIBINFO_C62_H */
