/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/*
 * Contract tests for the AX.25 -> aprsPacket parser. Frames are synthesised
 * here rather than demodulated so each test can pin one property of the
 * parser exactly: address unshifting, SSIDs, digipeater paths, DTI
 * classification, addressed-message unwrapping, and the malformed frames that
 * must be rejected instead of parsed into garbage.
 */

#include <catch2/catch_test_macros.hpp>

#include <cerrno>
#include <cstring>

#include "aprs_test_frame.h"
#include "protocols/APRS/packet.h"

TEST_CASE("APRS packet: destination and source are unshifted", "[aprs]")
{
    struct aprsTestFrame frame = aprsTest_makeFrame("APRS", 0, "N2BP", 7,
                                                    ">hello");
    aprsPacket pkt;

    REQUIRE(aprsPkt_fromFrame(frame.data, frame.len, &pkt) == 0);
    REQUIRE(pkt.addressesLen == 2);
    REQUIRE(strcmp(pkt.addresses[0].addr, "APRS") == 0);
    REQUIRE(pkt.addresses[0].ssid == 0);
    REQUIRE(strcmp(pkt.addresses[1].addr, "N2BP") == 0);
    REQUIRE(pkt.addresses[1].ssid == 7);
    REQUIRE(memcmp(pkt.info, ">hello", pkt.infoLen) == 0);
    REQUIRE(pkt.infoLen == 6);
}

TEST_CASE("APRS packet: address formatting follows the TNC2 convention",
          "[aprs]")
{
    struct aprsTestFrame frame = aprsTest_makeFrame("APRS", 0, "N2BP", 7,
                                                    ">hi");
    aprsPacket pkt;
    char buf[APRS_ADDR_STR_LEN];

    REQUIRE(aprsPkt_fromFrame(frame.data, frame.len, &pkt) == 0);

    /* SSID 0 is left off entirely. */
    REQUIRE(aprsAddr_toStr(&pkt.addresses[0], buf, sizeof(buf)) == 4);
    REQUIRE(strcmp(buf, "APRS") == 0);

    REQUIRE(aprsAddr_toStr(&pkt.addresses[1], buf, sizeof(buf)) == 6);
    REQUIRE(strcmp(buf, "N2BP-7") == 0);
}

TEST_CASE("APRS packet: the longest callsign-SSID pair still fits", "[aprs]")
{
    struct aprsTestFrame frame = aprsTest_makeFrame("APRS", 0, "ABCDEF", 15,
                                                    ">hi");
    aprsPacket pkt;
    char buf[APRS_ADDR_STR_LEN];

    REQUIRE(aprsPkt_fromFrame(frame.data, frame.len, &pkt) == 0);
    REQUIRE(aprsAddr_toStr(&pkt.addresses[1], buf, sizeof(buf)) == 9);
    REQUIRE(strcmp(buf, "ABCDEF-15") == 0);
}

TEST_CASE("APRS packet: digipeater path is formatted with repeat markers",
          "[aprs]")
{
    struct aprsTestFrame frame;
    memset(&frame, 0, sizeof(frame));

    aprsTest_putAddress(frame.data + 0, "APRS", 0, false, false);
    aprsTest_putAddress(frame.data + 7, "N2BP", 7, false, false);
    aprsTest_putAddress(frame.data + 14, "WIDE1", 1, true, false);
    aprsTest_putAddress(frame.data + 21, "WIDE2", 2, false, true);
    frame.data[28] = 0x03;
    frame.data[29] = 0xf0;
    memcpy(frame.data + 30, ">hi", 3);
    frame.len = 33;

    aprsPacket pkt;
    REQUIRE(aprsPkt_fromFrame(frame.data, frame.len, &pkt) == 0);
    REQUIRE(pkt.addressesLen == 4);

    char path[32];
    const size_t len = aprsPath_toStr(&pkt, path, sizeof(path));
    REQUIRE(len == strlen(path));
    REQUIRE(strcmp(path, "WIDE1-1*,WIDE2-2") == 0);
}

TEST_CASE("APRS packet: a directly heard packet has an empty path", "[aprs]")
{
    struct aprsTestFrame frame = aprsTest_makeFrame("APRS", 0, "N2BP", 7,
                                                    ">hi");
    aprsPacket pkt;
    char path[32];

    REQUIRE(aprsPkt_fromFrame(frame.data, frame.len, &pkt) == 0);
    REQUIRE(aprsPath_toStr(&pkt, path, sizeof(path)) == 0);
    REQUIRE(path[0] == '\0');
}

TEST_CASE("APRS packet: the data type identifier is classified", "[aprs]")
{
    struct {
        const char *info;
        enum aprsType type;
    } cases[] = {
        { ":N2BP-7   :hi", APRS_TYPE_MESSAGE },
        { "!4903.50N/07201.75W-", APRS_TYPE_POSITION },
        { "=4903.50N/07201.75W-", APRS_TYPE_POSITION },
        { "/092345z4903.50N/07201.75W>", APRS_TYPE_POSITION },
        { "@092345z4903.50N/07201.75W>", APRS_TYPE_POSITION },
        { ">on the air", APRS_TYPE_STATUS },
        { ";LEADER   *092345z4903.50N/07201.75W>", APRS_TYPE_OBJECT },
        { ")AID #2!4903.50N/07201.75W", APRS_TYPE_OBJECT },
        { "T#005,199,000,255,073,123,01101001", APRS_TYPE_TELEMETRY },
        { "?APRS?", APRS_TYPE_OTHER },
    };

    for (const auto &c : cases) {
        struct aprsTestFrame frame = aprsTest_makeFrame("APRS", 0, "N2BP", 7,
                                                        c.info);
        aprsPacket pkt;

        INFO("info field: " << c.info);
        REQUIRE(aprsPkt_fromFrame(frame.data, frame.len, &pkt) == 0);
        REQUIRE(pkt.type == c.type);
    }
}

TEST_CASE("APRS packet: an addressed message is split into addressee and text",
          "[aprs]")
{
    struct aprsTestFrame frame = aprsTest_makeFrame("APRS", 0, "N2BP", 7,
                                                    ":W1AW     :hello there");
    aprsPacket pkt;
    char addressee[APRS_ADDR_STR_LEN];
    char text[APRS_PACLEN];

    REQUIRE(aprsPkt_fromFrame(frame.data, frame.len, &pkt) == 0);
    REQUIRE(pkt.type == APRS_TYPE_MESSAGE);
    REQUIRE(
        aprsMsg_unwrap(&pkt, addressee, sizeof(addressee), text, sizeof(text))
        == 0);
    REQUIRE(strcmp(addressee, "W1AW") == 0);
    REQUIRE(strcmp(text, "hello there") == 0);
}

TEST_CASE("APRS packet: the acknowledgement sequence is stripped", "[aprs]")
{
    struct aprsTestFrame frame = aprsTest_makeFrame("APRS", 0, "N2BP", 7,
                                                    ":W1AW     :ping{042");
    aprsPacket pkt;
    char addressee[APRS_ADDR_STR_LEN];
    char text[APRS_PACLEN];

    REQUIRE(aprsPkt_fromFrame(frame.data, frame.len, &pkt) == 0);
    REQUIRE(
        aprsMsg_unwrap(&pkt, addressee, sizeof(addressee), text, sizeof(text))
        == 0);
    REQUIRE(strcmp(addressee, "W1AW") == 0);
    REQUIRE(strcmp(text, "ping") == 0);
}

TEST_CASE("APRS packet: a full-width addressee keeps every character", "[aprs]")
{
    struct aprsTestFrame frame = aprsTest_makeFrame("APRS", 0, "N2BP", 7,
                                                    ":ABCDEF-15:x");
    aprsPacket pkt;
    char addressee[APRS_ADDR_STR_LEN];
    char text[APRS_PACLEN];

    REQUIRE(aprsPkt_fromFrame(frame.data, frame.len, &pkt) == 0);
    REQUIRE(
        aprsMsg_unwrap(&pkt, addressee, sizeof(addressee), text, sizeof(text))
        == 0);
    REQUIRE(strcmp(addressee, "ABCDEF-15") == 0);
    REQUIRE(strcmp(text, "x") == 0);
}

TEST_CASE("APRS packet: an empty message body unwraps to an empty string",
          "[aprs]")
{
    struct aprsTestFrame frame = aprsTest_makeFrame("APRS", 0, "N2BP", 7,
                                                    ":W1AW     :");
    aprsPacket pkt;
    char addressee[APRS_ADDR_STR_LEN];
    char text[APRS_PACLEN];

    REQUIRE(aprsPkt_fromFrame(frame.data, frame.len, &pkt) == 0);
    REQUIRE(
        aprsMsg_unwrap(&pkt, addressee, sizeof(addressee), text, sizeof(text))
        == 0);
    REQUIRE(strcmp(addressee, "W1AW") == 0);
    REQUIRE(text[0] == '\0');
}

TEST_CASE("APRS packet: unwrapping rejects info fields that only look like "
          "messages",
          "[aprs]")
{
    aprsPacket pkt;
    char addressee[APRS_ADDR_STR_LEN];
    char text[APRS_PACLEN];

    SECTION("no separator at the fixed offset")
    {
        struct aprsTestFrame frame = aprsTest_makeFrame("APRS", 0, "N2BP", 7,
                                                        ":not a message");
        REQUIRE(aprsPkt_fromFrame(frame.data, frame.len, &pkt) == 0);
        REQUIRE(pkt.type == APRS_TYPE_MESSAGE);
        REQUIRE(aprsMsg_unwrap(&pkt, addressee, sizeof(addressee), text,
                               sizeof(text))
                == -EINVAL);
    }

    SECTION("too short to hold an addressee")
    {
        struct aprsTestFrame frame = aprsTest_makeFrame("APRS", 0, "N2BP", 7,
                                                        ":W1AW");
        REQUIRE(aprsPkt_fromFrame(frame.data, frame.len, &pkt) == 0);
        REQUIRE(aprsMsg_unwrap(&pkt, addressee, sizeof(addressee), text,
                               sizeof(text))
                == -EINVAL);
    }

    SECTION("blank addressee")
    {
        struct aprsTestFrame frame = aprsTest_makeFrame("APRS", 0, "N2BP", 7,
                                                        ":         :hi");
        REQUIRE(aprsPkt_fromFrame(frame.data, frame.len, &pkt) == 0);
        REQUIRE(aprsMsg_unwrap(&pkt, addressee, sizeof(addressee), text,
                               sizeof(text))
                == -EINVAL);
    }

    SECTION("not a message at all")
    {
        struct aprsTestFrame frame = aprsTest_makeFrame("APRS", 0, "N2BP", 7,
                                                        ">status text");
        REQUIRE(aprsPkt_fromFrame(frame.data, frame.len, &pkt) == 0);
        REQUIRE(aprsMsg_unwrap(&pkt, addressee, sizeof(addressee), text,
                               sizeof(text))
                == -EINVAL);
    }
}

TEST_CASE("APRS packet: malformed frames are rejected", "[aprs]")
{
    aprsPacket pkt;

    SECTION("too short for two addresses")
    {
        struct aprsTestFrame frame;
        memset(&frame, 0, sizeof(frame));
        aprsTest_putAddress(frame.data, "APRS", 0, false, true);
        frame.len = 7;
        REQUIRE(aprsPkt_fromFrame(frame.data, frame.len, &pkt) == -EINVAL);
    }

    SECTION("address field is never terminated")
    {
        struct aprsTestFrame frame;
        memset(&frame, 0, sizeof(frame));
        for (uint8_t i = 0; i < 4; i++)
            aprsTest_putAddress(frame.data + i * 7, "NOEND", 0, false, false);
        frame.len = 30;
        REQUIRE(aprsPkt_fromFrame(frame.data, frame.len, &pkt) == -EINVAL);
    }

    SECTION("more addresses than AX.25 allows")
    {
        struct aprsTestFrame frame;
        memset(&frame, 0, sizeof(frame));
        for (uint8_t i = 0; i < APRS_MAX_ADDRESSES + 1; i++)
            aprsTest_putAddress(frame.data + i * 7, "RELAY", 0, false, false);
        frame.len = 7 * (APRS_MAX_ADDRESSES + 1);
        REQUIRE(aprsPkt_fromFrame(frame.data, frame.len, &pkt) == -EINVAL);
    }

    SECTION("no room left for an info field")
    {
        struct aprsTestFrame frame = aprsTest_makeFrame("APRS", 0, "N2BP", 7,
                                                        "");
        REQUIRE(frame.len == 16);
        REQUIRE(aprsPkt_fromFrame(frame.data, frame.len, &pkt) == -EINVAL);
    }
}

TEST_CASE("APRS packet: the info field may be as long as AX.25 allows",
          "[aprs]")
{
    char info[APRS_MAX_INFO + 2];
    struct aprsPacket pkt;

    memset(info, 'x', sizeof(info) - 1);
    info[0] = '>';

    SECTION("256 bytes are kept whole")
    {
        info[APRS_MAX_INFO] = '\0';
        struct aprsTestFrame frame = aprsTest_makeFrame("APORT1", 0, "K1ABC", 9,
                                                        info);

        REQUIRE(aprsPkt_fromFrame(frame.data, frame.len, &pkt) == 0);
        CHECK(pkt.infoLen == APRS_MAX_INFO);
        CHECK(memcmp(pkt.info, info, APRS_MAX_INFO) == 0);
    }

    SECTION("257 bytes are rejected")
    {
        info[APRS_MAX_INFO + 1] = '\0';
        struct aprsTestFrame frame = aprsTest_makeFrame("APORT1", 0, "K1ABC", 9,
                                                        info);

        REQUIRE(aprsPkt_fromFrame(frame.data, frame.len, &pkt) == -EINVAL);
    }
}
