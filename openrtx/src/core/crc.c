/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "core/crc.h"

uint16_t crc_m17(const void *data, const size_t len)
{
    static const uint16_t CRC_M17_TABLE[] = { 0x0000, 0x5935, 0xb26a, 0xeb5f,
                                              0x3de1, 0x64d4, 0x8f8b, 0xd6be,
                                              0x7bc2, 0x22f7, 0xc9a8, 0x909d,
                                              0x4623, 0x1f16, 0xf449, 0xad7c };
    uint16_t crc = 0xFFFF;
    const uint8_t *d = (const uint8_t *)data;

    for (size_t i = 0; i < len; i++) {
        uint8_t tbl_idx = (crc >> 12) ^ (d[i] >> 4);
        crc = CRC_M17_TABLE[tbl_idx & 0x0f] ^ (crc << 4);
        tbl_idx = (crc >> 12) ^ d[i];
        crc = CRC_M17_TABLE[tbl_idx & 0x0f] ^ (crc << 4);
    }
    return crc;
}

uint16_t crc_ccitt(const void *data, const size_t len)
{
    uint16_t x = 0;
    uint16_t crc = 0;
    const uint8_t *buf = ((const uint8_t *)data);

    for (size_t i = 0; i < len; i++) {
        x = (crc >> 8) ^ buf[i];
        x ^= x >> 4;
        crc = (crc << 8) ^ (x << 12) ^ (x << 5) ^ x;
    }

    return crc;
}

uint16_t crc_hdlc(const void *data, size_t len)
{
    uint16_t crc = 0xffff;
    uint16_t x = 0;
    const uint8_t *buf = ((const uint8_t *)data);

    for (size_t i = 0; i < len; i++) {
        x = (crc ^ buf[i]) & 0xff;
        x ^= (x << 4) & 0xff;
        crc = (crc >> 8) ^ (x << 8) ^ (x << 3) ^ (x >> 4);
    }

    return ~crc;
}
