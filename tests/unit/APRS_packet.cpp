/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <catch2/catch_test_macros.hpp>

#include "protocols/APRS/constants.h"
#include "protocols/APRS/packet.h"
#include "protocols/APRS/packet_list.h"
#include "core/crc.h"
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

size_t createFrameData(const uint8_t num, uint8_t *frame)
{
    char info[85];
    char dst[] = "APRS";
    uint8_t dstSSID = 0;
    char src[] = "N2BP";
    uint8_t srcSSID = 9;
    char addressee[] = "N2BP";
    uint16_t crc;
    size_t pktSize;

    createAddress(dst, dstSSID, true, false, frame + 0);
    createAddress(src, srcSSID, false, true, frame + 7);
    frame[14] = 0x03; // UI-frame
    frame[15] = 0xf0; // no layer 3 protocol
    sprintf(info, ":%-9s:Testing %d", addressee, num);
    memcpy(frame + 16, info, strlen(info));
    pktSize = 16 + strlen(info);
    crc = crc_hdlc(frame, pktSize);
    frame[pktSize] = crc & 0xFF;
    frame[pktSize + 1] = (crc >> 8) & 0xFF;
    pktSize += 2;

    return pktSize;
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
    uint8_t frame[APRS_PACLEN];
    size_t len;

    len = createFrameData(1, frame);

    aprsPacket *pkt = aprsPktFromFrame(frame, len);
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
    struct aprsPacket *pkts[5];
    uint8_t frame[APRS_PACLEN];
    uint8_t i;

    // initialize the list
    aprsPktList_init(&list);
    REQUIRE(list.head == NULL);
    REQUIRE(list.tail == NULL);
    REQUIRE(list.len == 0);

    // add packets to the list
    for (i = 0; i < 5; i++) {
        size_t len = createFrameData(i, frame);
        pkts[i] = aprsPktFromFrame(frame, len);
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
    uint8_t frame[APRS_PACLEN];
    struct aprsPacket *pkts[10];
    uint8_t i;

    // initialize the lists
    aprsPktList_init(&list1);
    aprsPktList_init(&list2);

    // add packets to list1
    for (i = 0; i < 5; i++) {
        size_t len = createFrameData(i, frame);
        pkts[i] = aprsPktFromFrame(frame, len);
        list1 = aprsPktList_insert(list1, pkts[i]);
    }

    // add packets to list2
    for (; i < 10; i++) {
        size_t len = createFrameData(i, frame);
        pkts[i] = aprsPktFromFrame(frame, len);
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

/* ------------------------------------------------------------------------ *
 * Frame construction.
 *
 * The builder is checked two ways. Round-tripping through the parser above
 * proves the two agree with each other, which is what the radio's own RX and
 * TX paths need. Agreeing with each other is not the same as agreeing with
 * the rest of the world, so the byte-for-byte vector below comes from
 * scripts/aprs_gen_baseband.py — an independent implementation written
 * against the AX.25 specification, and the one whose output the RX e2e test
 * feeds to the demodulator.
 *
 * aprsFrameBuild() produces a frame WITHOUT the frame check sequence (the
 * modulator appends it on transmit), while aprsPktFromFrame() expects the FCS
 * to be present, so the round-trip appends crc_hdlc() before parsing.
 * ------------------------------------------------------------------------ */

/** Build a frame, append its FCS, and parse it back — the caller frees. */
static struct aprsPacket *buildAndParse(const char *dst, const char *src,
                                        const char *path, const char *info)
{
    uint8_t frame[APRS_PACLEN];
    size_t len = aprsFrameBuild(frame, sizeof(frame), dst, src, path, info,
                                strlen(info));
    if (len == 0)
        return NULL;

    uint16_t crc = crc_hdlc(frame, len);
    frame[len] = crc & 0xFF;
    frame[len + 1] = (crc >> 8) & 0xFF;

    return aprsPktFromFrame(frame, len + 2);
}

TEST_CASE("APRS address text parses back into an address", "[aprs][packet]")
{
    struct aprsAddress addr;

    REQUIRE(aprsAddrFromStr("W1AW", &addr) == true);
    REQUIRE(std::string(addr.addr) == std::string("W1AW"));
    REQUIRE(addr.ssid == 0);

    REQUIRE(aprsAddrFromStr("N0CALL-15", &addr) == true);
    REQUIRE(std::string(addr.addr) == std::string("N0CALL"));
    REQUIRE(addr.ssid == 15);

    /* lower case is folded up so a typed recipient still works */
    REQUIRE(aprsAddrFromStr("n0call-7", &addr) == true);
    REQUIRE(std::string(addr.addr) == std::string("N0CALL"));
    REQUIRE(addr.ssid == 7);

    /* malformed text is rejected */
    REQUIRE(aprsAddrFromStr("", &addr) == false);
    REQUIRE(aprsAddrFromStr("-7", &addr) == false);
    REQUIRE(aprsAddrFromStr("N0CALL-", &addr) == false);
    REQUIRE(aprsAddrFromStr("N0CALL-16", &addr) == false);
    REQUIRE(aprsAddrFromStr("N0CALL-x", &addr) == false);
    REQUIRE(aprsAddrFromStr("TOOLONGCALL", &addr) == false);
    REQUIRE(aprsAddrFromStr("WITH SPACE", &addr) == false);
    REQUIRE(aprsAddrFromStr(NULL, &addr) == false);
    REQUIRE(aprsAddrFromStr("W1AW", NULL) == false);
}

TEST_CASE("APRS frame builder round-trips through the parser", "[aprs][packet]")
{
    struct aprsPacket *pkt = buildAndParse(APRS_TOCALL, "N0CALL-7",
                                           APRS_DEFAULT_PATH, ":W1AW     :hi");
    REQUIRE(pkt != NULL);

    REQUIRE(pkt->addressesLen == 4);
    REQUIRE(std::string(pkt->addresses[0].addr) == std::string(APRS_TOCALL));
    REQUIRE(std::string(pkt->addresses[1].addr) == std::string("N0CALL"));
    REQUIRE(pkt->addresses[1].ssid == 7);
    REQUIRE(std::string(pkt->addresses[2].addr) == std::string("WIDE1"));
    REQUIRE(pkt->addresses[2].ssid == 1);
    REQUIRE(std::string(pkt->addresses[3].addr) == std::string("WIDE2"));
    REQUIRE(pkt->addresses[3].ssid == 1);
    REQUIRE(std::string(pkt->info) == std::string(":W1AW     :hi"));

    free(pkt);
}

TEST_CASE("APRS frame builder handles an absent path", "[aprs][packet]")
{
    struct aprsPacket *pkt = buildAndParse(APRS_TOCALL, "N0CALL", NULL,
                                           ">heard direct");
    REQUIRE(pkt != NULL);
    REQUIRE(pkt->addressesLen == 2);
    REQUIRE(std::string(pkt->info) == std::string(">heard direct"));
    free(pkt);

    pkt = buildAndParse(APRS_TOCALL, "N0CALL", "", ">heard direct");
    REQUIRE(pkt != NULL);
    REQUIRE(pkt->addressesLen == 2);
    free(pkt);
}

TEST_CASE("APRS frame builder matches an independent encoder", "[aprs][packet]")
{
    /* scripts/aprs_gen_baseband.py, parse_tnc2() of
     *   N0CALL-7>APORT1,WIDE1-1,WIDE2-1::W1AW     :hello
     * which produces the frame without a frame check sequence. */
    static const uint8_t expected[] = {
        0x82, 0xa0, 0x9e, 0xa4, 0xa8, 0x62, 0xe0, 0x9c, 0x60, 0x86, 0x82, 0x98,
        0x98, 0x6e, 0xae, 0x92, 0x88, 0x8a, 0x62, 0x40, 0x62, 0xae, 0x92, 0x88,
        0x8a, 0x64, 0x40, 0x63, 0x03, 0xf0, 0x3a, 0x57, 0x31, 0x41, 0x57, 0x20,
        0x20, 0x20, 0x20, 0x20, 0x3a, 0x68, 0x65, 0x6c, 0x6c, 0x6f
    };

    uint8_t frame[APRS_PACLEN];
    const char info[] = ":W1AW     :hello";
    size_t len = aprsFrameBuild(frame, sizeof(frame), APRS_TOCALL, "N0CALL-7",
                                APRS_DEFAULT_PATH, info, strlen(info));

    REQUIRE(len == sizeof(expected));
    REQUIRE(memcmp(frame, expected, sizeof(expected)) == 0);
}

TEST_CASE("APRS frame builder rejects invalid arguments", "[aprs][packet]")
{
    uint8_t frame[APRS_PACLEN];
    const char info[] = ">hi";

    REQUIRE(aprsFrameBuild(NULL, sizeof(frame), APRS_TOCALL, "N0CALL", NULL,
                           info, strlen(info))
            == 0);
    REQUIRE(aprsFrameBuild(frame, sizeof(frame), APRS_TOCALL, "N0CALL-99", NULL,
                           info, strlen(info))
            == 0);
    REQUIRE(aprsFrameBuild(frame, sizeof(frame), APRS_TOCALL, "N0CALL",
                           "WIDE1-1,", info, strlen(info))
            == 0);
    /* an empty info field would not parse back */
    REQUIRE(aprsFrameBuild(frame, sizeof(frame), APRS_TOCALL, "N0CALL", NULL,
                           info, 0)
            == 0);
    /* a frame that would not fit */
    uint8_t big[APRS_PACLEN];
    memset(big, 'x', sizeof(big));
    REQUIRE(aprsFrameBuild(frame, sizeof(frame), APRS_TOCALL, "N0CALL",
                           APRS_DEFAULT_PATH, (const char *)big, sizeof(big))
            == 0);
}
