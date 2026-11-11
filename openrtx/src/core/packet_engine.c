/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "core/packet_engine.h"
#include "core/slip.h"
#include "interfaces/vcom.h"

/** KISS command byte of a data frame on port 0. */
#define KISS_DATA_FRAME 0x00

int packetEngine_send(const uint8_t *frame, size_t len)
{
    const size_t kissLen = len + 1;
    // Every byte may need escaping, and SLIP adds a delimiter at each end
    const size_t slipMax = (2 * kissLen) + 2;
    struct slip sl;
    int ret;

    // The KISS frame, then room for its SLIP encoding
    uint8_t *kiss = malloc(kissLen + slipMax);
    if (kiss == NULL)
        return -ENOMEM;

    uint8_t *encoded = kiss + kissLen;
    kiss[0] = KISS_DATA_FRAME;
    memcpy(&kiss[1], frame, len);

    slip_init(&sl, kiss, kissLen);
    ret = slip_encode(&sl, encoded, slipMax);
    if (ret > 0) {
        const ssize_t written = vcom_writeBlock(encoded, ret);
        ret = (written < 0) ? (int)written : 0;
    }

    free(kiss);
    return ret;
}
