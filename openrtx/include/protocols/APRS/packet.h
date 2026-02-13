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

#ifdef __cplusplus
extern "C" {
#endif

typedef struct address {
    char addr[7]; /* include null termination */
    uint8_t ssid : 4;
    bool commandHeard;
    struct address *next;
} aprsAddress_t;

typedef struct packet {
    aprsAddress_t *addresses;
    char *info;
    datetime_t ts;
    struct packet *prev;
    struct packet *next;
} aprsPacket_t;

void aprsPktFree(aprsPacket_t *pkt);
aprsPacket_t *aprsPktsInsert(aprsPacket_t *head, aprsPacket_t *pkt);
aprsPacket_t *aprsPktDelete(aprsPacket_t *head, aprsPacket_t *pkt);
aprsPacket_t *aprsPktFromFrame(uint8_t *frame, size_t frameSize);

#ifdef __cplusplus
}
#endif

#endif /* APRS_PACKET_H */
