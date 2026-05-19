/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CRC_H
#define CRC_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Compute the M17 16-bit CRC over a given block of data.
 *
 * Created using pycrc https://pycrc.org with the configuration:
 *  - Width         = 16
 *  - Poly          = 0x5935
 *  - XorIn         = 0xffff
 *  - ReflectIn     = False
 *  - XorOut        = 0x0000
 *  - ReflectOut    = False
 *  - Algorithm     = table-driven
 *
 * @param data: input data.
 * @param len: data length, in bytes.
 * @return A uint16_t M17 CRC.
 */
uint16_t crc_m17(const void *data, const size_t len);

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
