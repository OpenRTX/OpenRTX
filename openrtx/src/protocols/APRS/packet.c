/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "protocols/APRS/packet.h"
#include "interfaces/platform.h"

struct aprsPacket *aprsPktFromFrame(struct frameData *frame)
{
    uint8_t frameOffset = 0;
    bool lastAddress = false;

    DEBUG_PRINT("aprsPktFromFrame frame: ");
    for (uint8_t i = 0; i < frame->len; i++) {
        DEBUG_PRINT("%02x ", frame->data[i]);
    }
    DEBUG_PRINT("\n");
    DEBUG_PRINT("frame->len=%d\n", frame->len);

    /* calculate the size of the addresses */
    uint8_t addressesLen = 0;
    while (((frame->len - frameOffset) >= 7) && !lastAddress) {
        addressesLen++;
        lastAddress = frame->data[frameOffset + 6] & 1;
        frameOffset += 7;
    }

    DEBUG_PRINT("addressesLen=%d frameOffset=%d\n", addressesLen, frameOffset);

    /* skip the control field and protocol ID */
    frameOffset += 2;

    DEBUG_PRINT("frameOffset=%d\n", frameOffset);

    /* calculate the size of the info section */
    uint8_t infoLen = frame->len - frameOffset + 1;

    DEBUG_PRINT("infoLen=%d\n", infoLen);

    /* allocate memory for a new packet and initialize */
    struct aprsPacket *pkt = (struct aprsPacket *)malloc(
        sizeof(struct aprsPacket) + sizeof(struct aprsAddress) * addressesLen
        + infoLen);
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
    // pointer to the variable length part of the struct
    uint8_t *endPtr = (uint8_t *)pkt + sizeof(struct aprsPacket);
    pkt->addressesLen = addressesLen;
    pkt->addresses = (struct aprsAddress *)endPtr;
    pkt->infoLen = infoLen;
    pkt->info = (char *)(endPtr + sizeof(struct aprsAddress) * addressesLen);

    /* copy address from front of frame data into our packet addresses */
    frameOffset = 0;
    lastAddress = false;
    uint8_t addressIndex = 0;
    /* addresses are 7 bytes long */
    while (((frame->len - frameOffset) >= 7) && !lastAddress) {
        memset(pkt->addresses[addressIndex].addr, 0, 7);
        uint8_t j = 0;
        for (uint8_t i = 0; i < 6; i++) {
            /* address characters are ASCII that has been left shifted once */
            uint8_t character = frame->data[frameOffset + i] >> 1;
            /* include only printable characters */
            if ((character > 32) && (character < 127))
                pkt->addresses[addressIndex].addr[j++] = character;
        }
        /* the last byte is an SSID and some flags */
        pkt->addresses[addressIndex].ssid = (frame->data[frameOffset + 6] & 30)
                                         >> 1;
        pkt->addresses[addressIndex].commandHeard =
            (frame->data[frameOffset + 6] & 0x80) >> 7;
        lastAddress = frame->data[frameOffset + 6] & 1;
        frameOffset += 7;
        addressIndex++;
    }

    DEBUG_PRINT("frameOffset=%d\n", frameOffset);

    /* skip the control field and protocol ID */
    frameOffset += 2;

    DEBUG_PRINT("frameOffset=%d\n", frameOffset);

    /* copy info into the packet */
    memcpy(pkt->info, frame->data + frameOffset, infoLen - 1);
    pkt->info[infoLen - 1] = '\0';

    return pkt;
}
