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
 * Compute the CRC-16/XMODEM CRC over a given block of data.
 *
 * width=16, poly=1021, init=0x0000, refin=false, refout=false, xorout=0x0000,
 * check=0x31c3 residue=0x000 name="CRC-16/XMODEM"
 *
 * @param data: input data.
 * @param len: data length, in bytes.
 * @return CRC-16/XMODEM CRC.
 */
uint16_t crc_ccitt(const void *data, const size_t len);

/**
 * Compute the CRC-16/IBM-SDLC CRC over a given block of data.
 *
 * width=16, poly=1021, init=0xffff, refin=true, refout=true, xorout=0xffff,
 * check=0x906e, residue=0xf0b8, name="CRC-16/IBM-SDLC"
 *
 * @param data: input data.
 * @param len: data length, in bytes.
 * @return HDLC CRC.
 */
uint16_t crc_hdlc(const void *data, const size_t len);

#ifdef __cplusplus
}
#endif

#endif /* CRC_H */
