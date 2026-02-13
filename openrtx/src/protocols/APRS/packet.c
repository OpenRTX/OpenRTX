/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "protocols/APRS/packet.h"
#include "interfaces/platform.h"

/**
 * @brief Frees memory allocated to make an APRS packet
 *
 * When packets are created an aprsPacket_t struct, aprsAddress_t structs, and
 * an info array is allocated. This deallocates them.
 *
 * @param pkt a pointer to an aprsPacket_t
 */
void aprsPktFree(aprsPacket_t *pkt)
{
    if (!pkt)
        return;

    aprsAddress_t *nextAddress;
    for (aprsAddress_t *address = pkt->addresses; address;
         address = nextAddress) {
        nextAddress = address->next;
        free(address);
    }
    if (pkt->info)
        free(pkt->info);
    free(pkt);
}

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
aprsPacket_t *aprsPktsInsert(aprsPacket_t *head, aprsPacket_t *pkt)
{
    aprsPacket_t *last;
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
aprsPacket_t *aprsPktDelete(aprsPacket_t *head, aprsPacket_t *pkt)
{
    aprsPacket_t *newHead;

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

aprsPacket_t *aprsPktFromFrame(uint8_t *frame, size_t frameSize)
{
    /* allocate memory for a new packet */
    aprsPacket_t *pkt = (aprsPacket_t *)malloc(sizeof(aprsPacket_t));
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

    /* pull the addresses from the front of the frame and make aprsAddress_t's
     * for them */
    size_t frameOffset = 0;
    bool lastAddress = false;
    aprsAddress_t *prevAddress = NULL;
    /* addresses are 7 bytes long */
    while (((frameSize - frameOffset) > 6) && !lastAddress) {
        /* create and initialize an aprsAddress_t */
        aprsAddress_t *address = (aprsAddress_t *)malloc(sizeof(aprsAddress_t));
        memset(address->addr, 0, 7);
        address->ssid = 0;
        address->next = NULL;
        /* an aprsPacket_t holds a linked list of aprsAddress_t */
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
