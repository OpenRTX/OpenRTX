/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef APRS_PACKET_H
#define APRS_PACKET_H

#include <string.h>
#include <stdlib.h>
#include "interfaces/platform.h"
#include "protocols/APRS/frame.h"

#ifdef __cplusplus
extern "C" {
#endif

struct aprsAddress {
    char addr[7]; /* include null termination */
    uint8_t ssid         : 4;
    uint8_t commandHeard : 1;
    uint8_t __unused     : 3;
};

struct aprsPacket {
    struct aprsPacket *prev;
    struct aprsPacket *next;
    datetime_t ts;
    uint8_t addressesLen;
    uint8_t infoLen;
    struct aprsAddress *addresses;
    char *info;
};

/**
 * @brief Creates an APRS packet from frameData
 *
 * @param frame A struct frameData of data
 * @return An aprsPacket pointer to a packet
 */
struct aprsPacket *aprsPktFromFrame(struct frameData *frame);

#ifdef __cplusplus
}
#endif

#endif /* APRS_PACKET_H */
