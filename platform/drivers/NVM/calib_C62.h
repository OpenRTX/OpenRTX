/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CALIB_C62_H
#define CALIB_C62_H

#include "calibration/calibInfo_C62.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

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

#ifdef __cplusplus
}
#endif

#endif /* CALIB_C62_H */
