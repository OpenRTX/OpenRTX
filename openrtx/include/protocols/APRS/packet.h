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

struct aprsAddress {
    struct aprsAddress *next;
    char addr[7]; /* include null termination */
    uint8_t ssid      : 4;
    bool commandHeard : 1;
    uint8_t __unused  : 3;
};

struct aprsPacket {
    struct aprsAddress *addresses;
    char *info;
    datetime_t ts;
    struct aprsPacket *prev;
    struct aprsPacket *next;
};

/**
 * @brief Frees memory allocated to make an APRS packet
 *
 * When packets are created an aprsPacket struct, aprsAddress structs, and
 * an info array is allocated. This deallocates them.
 *
 * @param pkt a pointer to an aprsPacket
 */
void aprsPktFree(struct aprsPacket *pkt);

/**
 * @brief Inserts APRS packets at the front of a double-linked list
 *
 * APRS packets are stored as a double-linked list in RTX and in state. This
 * function adds packets (one or many) to the FRONT of a list.
 *
 * @param head The current head of the list
 * @param pkt The packet to add
 * @return The new head of the list
 */
struct aprsPacket *aprsPktsInsert(struct aprsPacket *head,
                                  struct aprsPacket *pkt);

/**
 * @brief Deletes an APRS packet from a list
 *
 * APRS packets are stored as a double-linked list. This function deletes a
 * packet and keeps, fixes the pointers around it, and frees the memory
 * allocated.
 *
 * @param head The current head of the list
 * @param pkt The packet to delete
 * @return The new head of the list
 */
struct aprsPacket *aprsPktDelete(struct aprsPacket *head,
                                 struct aprsPacket *pkt);

/**
 * @brief Creates an APRS packet from frameData
 *
 * @param frame A uint8_t array of data
 * @param frameSize The size of the data
 * @return An aprsPacket pointer to a packet
 */
struct aprsPacket *aprsPktFromFrame(uint8_t *frame, size_t frameSize);

#ifdef __cplusplus
}
#endif

#endif /* APRS_PACKET_H */
