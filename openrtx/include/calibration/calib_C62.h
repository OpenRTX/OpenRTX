/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CALIB_C62_H
#define CALIB_C62_H

#include "core/datatypes.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

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

/**
 * Read the TX power calibration from the stock firmware's settings journal.
 *
 * The journal holds back-to-back settings entries, each starting with a 0x5A
 * marker, an 8-bit sum of its payload and its little-endian total length.
 * The newest entry with a valid checksum is used. Records outside the VHF
 * (136-174 MHz) and UHF (400-480 MHz) bands or above 100% duty are skipped.
 *
 * @param read: function reading len bytes at offset off of the journal into
 * buf, returning zero on success.
 * @param ctx: context passed to read.
 * @param size: journal size, in bytes.
 * @param cal: destination for the calibration data.
 * @return zero on success, -1 if the journal has no usable calibration.
 */
int calibC62_read(int (*read)(void *ctx, uint32_t off, void *buf, size_t len),
                  void *ctx, uint32_t size, struct C62Calib *cal);

/**
 * Fill the calibration with the stock firmware's default table.
 *
 * @param cal: destination for the calibration data.
 */
void calibC62_defaults(struct C62Calib *cal);

/**
 * Interpolate the calibration of a band at a frequency, linearly between the
 * two nearest points. Below the first point and above the last one, the
 * nearest point is used.
 *
 * @param band: band calibration with at least one point.
 * @param freq: frequency, in Hz.
 * @return calibration point at the frequency.
 */
struct C62CalPoint calibC62_interp(const struct C62BandCalib *band,
                                   freq_t freq);

#ifdef __cplusplus
}
#endif

#endif /* CALIB_C62_H */
