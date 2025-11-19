/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2020-2025 OpenRTX Contributors
 *
 * This file is part of OpenRTX.
 */

#ifndef CRC_H
#define CRC_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Compute the CCITT 16-bit CRC over a given block of data.
 *
 * @param data: input data.
 * @param len: data length, in bytes.
 * @return CCITT CRC.
 */
uint16_t crc_ccitt(const void *data, const size_t len);

#ifdef __cplusplus
}
#endif

#endif /* CRC_H */
