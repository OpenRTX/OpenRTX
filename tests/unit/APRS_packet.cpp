/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <catch2/catch_test_macros.hpp>

#include "protocols/APRS/constants.h"
#include "protocols/APRS/packet.h"
#include "protocols/APRS/packet_list.h"
#include "protocols/APRS/frame.h"
#include <cstdio>

void createAddress(const char *call, uint8_t ssid, bool command, bool last,
                   uint8_t *addr)
{
    uint8_t i = 0;

    // copy over address, shifting left one
    for (; i < 6; i++) {
        if (call[i] == '\0')
            break;
        addr[i] = call[i] << 1;
    }

    // fill unused bytes with spaces
    for (; i < 6; i++)
        addr[i] = ' ' << 1;

    // last byte is:
    // 0bCRRSSSSL: (C)ommand/response, (R)eserved (usually 1s), (S)SID,
    //             and (L)ast address flag
    addr[6] = 0;
    if (command)
        addr[6] |= 0b10000000;
    addr[6] |= 0b01100000;
    addr[6] |= (ssid & 0x0F) << 1;
    if (last)
        addr[6] |= 1;
}

struct frameData createFrameData(const uint8_t num, struct frameData frame)
{
    char info[85];
    char dst[] = "APRS";
    uint8_t dstSSID = 0;
    char src[] = "N2BP";
    uint8_t srcSSID = 9;
    char addressee[] = "N2BP";

    createAddress(dst, dstSSID, true, false, frame.data + 0);
    createAddress(src, srcSSID, false, true, frame.data + 7);
    frame.data[14] = 0x03; // UI-frame
    frame.data[15] = 0xf0; // no layer 3 protocol
    sprintf(info, ":%-9s:Testing %d", addressee, num);
    memcpy(frame.data + 16, info, strlen(info));
    frame.len = 16 + strlen(info);

    return frame;
}

void printPacket(struct aprsPacket *pkt)
{
    printf("aprsPacket %p: prev=%p, next=%p, addressLen=%d, infoLen=%d\n",
           (void *)pkt, (void *)pkt->prev, (void *)pkt->next, pkt->addressesLen,
           pkt->infoLen);
    printf("  ts: year=%d, month=%d, day=%d, hour=%d, minute=%d, second=%d\n",
           pkt->ts.year, pkt->ts.month, pkt->ts.day, pkt->ts.hour,
           pkt->ts.minute, pkt->ts.second);
    for (uint8_t i = 0; i < pkt->addressesLen; i++) {
        printf("  address[%d]: addr=%s, ssid=%d, commandHeard=%d\n", i,
               pkt->addresses[i].addr, pkt->addresses[i].ssid,
               pkt->addresses[i].commandHeard);
    }
    printf("  info=%s\n", pkt->info);
}

void printPacketList(struct aprsPktList list)
{
    printf("aprsPacketList: head=%p, tail=%p, len=%ld\n", (void *)list.head,
           (void *)list.tail, list.len);
    for (aprsPacket *pkt = list.head; pkt; pkt = pkt->next)
        printPacket(pkt);
}

TEST_CASE("APRS packets can be created from frame data", "[aprs][packet]")
{
    struct frameData frame;

    frame = createFrameData(1, frame);

    aprsPacket *pkt = aprsPktFromFrame(&frame);
    printPacket(pkt);

    REQUIRE(pkt->prev == NULL);
    REQUIRE(pkt->next == NULL);
    REQUIRE(pkt->addressesLen == 2);
    REQUIRE(pkt->infoLen == 21);
    REQUIRE(std::string(pkt->addresses[0].addr) == std::string("APRS"));
    REQUIRE(pkt->addresses[0].ssid == 0);
    REQUIRE(std::string(pkt->addresses[1].addr) == std::string("N2BP"));
    REQUIRE(pkt->addresses[1].ssid == 9);
    REQUIRE(std::string(pkt->info) == std::string(":N2BP     :Testing 1"));

    free(pkt);
}

TEST_CASE("APRS packet lists can be added to and deleted from",
          "[aprs][packet]")
{
    struct aprsPktList list;
    struct frameData frame;
    struct aprsPacket *pkts[5];
    uint8_t i;

    // initialize the list
    list = aprsPktList_init();
    REQUIRE(list.head == NULL);
    REQUIRE(list.tail == NULL);
    REQUIRE(list.len == 0);

    // add packets to the list
    for (i = 0; i < 5; i++) {
        frame = createFrameData(i, frame);
        pkts[i] = aprsPktFromFrame(&frame);
        list = aprsPktList_insert(list, pkts[i]);
    }
    REQUIRE(list.head == pkts[4]);
    REQUIRE(list.tail == pkts[0]);
    REQUIRE(list.len == 5);
    REQUIRE(list.head->prev == NULL);
    REQUIRE(list.tail->next == NULL);

    // move forward throught the list
    i = 4;
    for (aprsPacket *pkt = list.head; pkt; pkt = pkt->next)
        REQUIRE(pkt == pkts[i--]);

    // move backward throught the list
    i = 0;
    for (aprsPacket *pkt = list.tail; pkt; pkt = pkt->prev)
        REQUIRE(pkt == pkts[i++]);

    // delete the head
    list = aprsPktList_delete(list, pkts[4]);
    REQUIRE(list.head == pkts[3]);
    REQUIRE(list.tail == pkts[0]);
    REQUIRE(list.len == 4);
    REQUIRE(list.head->prev == NULL);
    REQUIRE(list.tail->next == NULL);

    // delete the tail
    list = aprsPktList_delete(list, pkts[0]);
    REQUIRE(list.head == pkts[3]);
    REQUIRE(list.tail == pkts[1]);
    REQUIRE(list.len == 3);
    REQUIRE(list.head->prev == NULL);
    REQUIRE(list.tail->next == NULL);

    // delete from middle
    list = aprsPktList_delete(list, pkts[2]);
    REQUIRE(list.head == pkts[3]);
    REQUIRE(list.tail == pkts[1]);
    REQUIRE(list.len == 2);
    REQUIRE(list.head->prev == NULL);
    REQUIRE(list.tail->next == NULL);
    REQUIRE(pkts[3]->next == pkts[1]);
    REQUIRE(pkts[1]->prev == pkts[3]);

    aprsPktList_release(list);
}

TEST_CASE("APRS packet lists can be concatenated", "[aprs][packet]")
{
    struct aprsPktList list1, list2, list3;
    struct frameData frame;
    struct aprsPacket *pkts[10];
    uint8_t i;

    // initialize the lists
    list1 = aprsPktList_init();
    list2 = aprsPktList_init();

    // add packets to list1
    for (i = 0; i < 5; i++) {
        frame = createFrameData(i, frame);
        pkts[i] = aprsPktFromFrame(&frame);
        list1 = aprsPktList_insert(list1, pkts[i]);
    }

    // add packets to list2
    for (; i < 10; i++) {
        frame = createFrameData(i, frame);
        pkts[i] = aprsPktFromFrame(&frame);
        list2 = aprsPktList_insert(list2, pkts[i]);
    }

    list3 = aprsPktList_concat(list1, list2);
    // result should be [4, 3, 2, 1, 0, 9, 8, 7, 6, 5]
    printPacketList(list3);

    REQUIRE(list3.head->prev == NULL);
    REQUIRE(list3.tail->next == NULL);

    // move forward throught the list
    i = 4;
    for (aprsPacket *pkt = list3.head; pkt; pkt = pkt->next) {
        REQUIRE(pkt == pkts[i]);
        i = (i == 0) ? 9 : i - 1;
    }

    // move backward throught the list
    i = 5;
    for (aprsPacket *pkt = list3.tail; pkt; pkt = pkt->prev) {
        REQUIRE(pkt == pkts[i]);
        i = (i == 9) ? 0 : i + 1;
    }

    aprsPktList_release(list3);
}
