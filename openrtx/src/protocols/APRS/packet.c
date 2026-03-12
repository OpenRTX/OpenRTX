/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "protocols/APRS/packet.h"
#include "interfaces/platform.h"

void aprsPktFree(struct aprsPacket *pkt)
{
    if (!pkt)
        return;

    struct aprsAddress *nextAddress;
    for (struct aprsAddress *address = pkt->addresses; address;
         address = nextAddress) {
        nextAddress = address->next;
        free(address);
    }
    if (pkt->info)
        free(pkt->info);
    free(pkt);
}

struct aprsPacket *aprsPktsInsert(struct aprsPacket *head,
                                  struct aprsPacket *pkt)
{
    struct aprsPacket *last;
    for (last = pkt; last->next; last = last->next)
        ;
    pkt->prev = NULL;
    if (!head) {
        last->next = NULL;
    } else {
        head->prev = last;
        last->next = head;
    }
    return pkt;
}

struct aprsPacket *aprsPktDelete(struct aprsPacket *head,
                                 struct aprsPacket *pkt)
{
    struct aprsPacket *newHead;

    // adjust the pointers
    if (pkt == head) {
        // it's the head
        if (!pkt->next) {
            // it's the only packet
            newHead = NULL;
        } else {
            // there's more packets after it
            newHead = pkt->next;
            newHead->prev = NULL;
        }
    } else if (!pkt->next) {
        // it's the tail
        newHead = head;
        pkt->prev->next = NULL;
    } else {
        // it's in the middle
        newHead = head;
        pkt->prev->next = pkt->next;
        pkt->next->prev = pkt->prev;
    }

    // free the packet's memory
    aprsPktFree(pkt);

    return newHead;
}

struct aprsPacket *aprsPktFromFrame(uint8_t *frame, size_t frameSize)
{
    /* allocate memory for a new packet */
    struct aprsPacket *pkt =
        (struct aprsPacket *)malloc(sizeof(struct aprsPacket));
    pkt->next = NULL;
    pkt->prev = NULL;
#ifdef CONFIG_RTC
    pkt->ts = platform_getCurrentTime();
#else
    pkt->ts.hour = 0;
    pkt->ts.minute = 0;
    pkt->ts.second = 0;
    pkt->ts.day = 0;
    pkt->ts.date = 0;
    pkt->ts.month = 0;
    pkt->ts.year = 0;
#endif

    /* pull the addresses from the front of the frame and make aprsAddress's
     * for them */
    size_t frameOffset = 0;
    bool lastAddress = false;
    struct aprsAddress *prevAddress = NULL;
    /* addresses are 7 bytes long */
    while (((frameSize - frameOffset) > 6) && !lastAddress) {
        /* create and initialize an aprsAddress */
        struct aprsAddress *address =
            (struct aprsAddress *)malloc(sizeof(struct aprsAddress));
        memset(address->addr, 0, 7);
        address->ssid = 0;
        address->next = NULL;
        /* an aprsPacket holds a linked list of aprsAddress's */
        if (!prevAddress)
            /* we are the head */
            pkt->addresses = address;
        else
            prevAddress->next = address;
        uint8_t addressIndex = 0;
        for (uint8_t i = 0; i < 6; i++) {
            /* address characters are ASCII that has been left shifted once */
            uint8_t character = frame[frameOffset + i] >> 1;
            /* include only printable characters */
            if ((character > 32) && (character < 127))
                address->addr[addressIndex++] = character;
        }
        /* the last byte is an SSID and some flags */
        address->ssid = (frame[frameOffset + 6] & 30) >> 1;
        address->commandHeard = (frame[frameOffset + 6] & 0x80) >> 7;
        lastAddress = frame[frameOffset + 6] & 1;
        prevAddress = address;
        frameOffset += 7;
    }

    /* skip the control field and protocol ID */
    frameOffset += 2;

    /* copy info into the packet */
    size_t infoSize = frameSize - frameOffset;
    pkt->info = (char *)malloc(infoSize + 1);
    memcpy(pkt->info, frame + frameOffset, infoSize);
    pkt->info[infoSize] = '\0';

    return pkt;
}
