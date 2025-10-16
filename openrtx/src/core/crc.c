/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "core/crc.h"

uint16_t crc_ccitt(const void *data, const size_t len)
{
    uint16_t x   = 0;
    uint16_t crc = 0;
    const uint8_t *buf = ((const uint8_t *) data);

    for(size_t i = 0; i < len; i++)
    {
        x   = (crc >> 8) ^ buf[i];
        x  ^= x >> 4;
        crc = (crc << 8) ^ (x << 12) ^ (x << 5) ^ x;
    }

    return crc;
}
