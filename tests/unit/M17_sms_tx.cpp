/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <cstring>
#include "rtx/SmsTxPacket.hpp"
#include "protocols/M17/PacketFramer.hpp"
#include "protocols/M17/FrameEncoder.hpp"
#include "protocols/M17/FrameDecoder.hpp"
#include "protocols/M17/Datatypes.hpp"
#include "core/crc.h"

using namespace M17;

// Helper: prepare SMS app data and assemble into frames.
static size_t buildSmsFrames(const char *msg, size_t msgLen, uint8_t *buffer,
                             size_t bufSize, PacketFrame *frames,
                             size_t maxFrames)
{
    size_t appLen = prepareSmsPacketData(msg, msgLen, buffer, bufSize);
    if (appLen == 0)
        return 0;

    PacketFramer framer;
    if (!framer.init(buffer, appLen))
        return 0;

    size_t n = 0;
    while (n < maxFrames && framer.nextFrame(frames[n]))
        n++;
    return n;
}

// ---------- Application-layer format tests ----------

TEST_CASE("SMS: empty message returns zero", "[m17][smstx]")
{
    uint8_t buf[32];
    REQUIRE(prepareSmsPacketData("", 0, buf, sizeof(buf)) == 0);
}

TEST_CASE("SMS: null message pointer returns zero", "[m17][smstx]")
{
    uint8_t buf[32];
    REQUIRE(prepareSmsPacketData(nullptr, 5, buf, sizeof(buf)) == 0);
}

TEST_CASE("SMS: protocol ID byte is 0x05", "[m17][smstx]")
{
    uint8_t buf[32];
    size_t len = prepareSmsPacketData("Hi", 2, buf, sizeof(buf));
    REQUIRE(len > 0);
    REQUIRE(buf[0] == 0x05);
}

TEST_CASE("SMS: message text follows protocol ID", "[m17][smstx]")
{
    uint8_t buf[32];
    prepareSmsPacketData("AB", 2, buf, sizeof(buf));
    REQUIRE(buf[1] == 'A');
    REQUIRE(buf[2] == 'B');
}

TEST_CASE("SMS: NUL terminator follows message", "[m17][smstx]")
{
    uint8_t buf[32];
    prepareSmsPacketData("AB", 2, buf, sizeof(buf));
    REQUIRE(buf[3] == 0x00);
}

TEST_CASE("SMS: CRC is computed over protocol ID + message + NUL",
          "[m17][smstx]")
{
    const char *msg = "Test CRC";
    size_t msgLen = strlen(msg);

    uint8_t buf[32];
    size_t appLen = prepareSmsPacketData(msg, msgLen, buf, sizeof(buf));
    REQUIRE(appLen == 1 + msgLen + 1);

    // Build frames to get CRC verification
    PacketFrame frames[4];
    size_t n = buildSmsFrames(msg, msgLen, buf, sizeof(buf), frames, 4);
    REQUIRE(n == 1);

    // Reconstruct expected CRC
    uint8_t appData[32];
    appData[0] = 0x05;
    memcpy(&appData[1], msg, msgLen);
    appData[1 + msgLen] = 0x00;

    uint16_t expected = crc_m17(appData, appLen);
    uint16_t actual = (static_cast<uint16_t>(frames[0][appLen]) << 8)
                    | frames[0][appLen + 1];
    REQUIRE(actual == expected);
}

// ---------- Integration: encode/decode round-trip ----------

TEST_CASE("SMS: round-trips through encoder/decoder", "[m17][smstx]")
{
    const char *msg = "OpenRTX";
    size_t msgLen = strlen(msg);

    uint8_t buf[32];
    PacketFrame frames[4];
    size_t n = buildSmsFrames(msg, msgLen, buf, sizeof(buf), frames, 4);
    REQUIRE(n == 1);

    // Encode
    FrameEncoder encoder;
    frame_t encoded;
    encoder.encodePacketFrame(frames[0], encoded);

    // Decode
    FrameDecoder decoder;
    FrameType type = decoder.decodeFrame(encoded);
    REQUIRE(type == FrameType::PACKET);

    const PacketFrame &decoded = decoder.getPacketFrame();
    REQUIRE(decoded[0] == 0x05);

    for (size_t i = 0; i < msgLen; i++)
        REQUIRE(decoded[1 + i] == static_cast<uint8_t>(msg[i]));

    REQUIRE(decoded[1 + msgLen] == 0x00);
}

// ---------- Reference vector ----------

TEST_CASE("SMS: reference vector matches known-good frame", "[m17][smstx]")
{
    // The known-good encoded frame for "OpenRTX" from M17_packet.cpp
    static constexpr frame_t KNOWN_GOOD_FRAME = {
        0x75, 0xFF, 0xF6, 0xD4, 0xC2, 0x59, 0x82, 0x96, 0x84, 0x63, 0x9A, 0x26,
        0xF6, 0xD8, 0xB8, 0xF0, 0x9D, 0x0D, 0x4C, 0xD0, 0x00, 0x03, 0x9F, 0x05,
        0xEE, 0x6E, 0x72, 0x2F, 0x23, 0xDA, 0x96, 0xFE, 0xCB, 0x70, 0x9B, 0x09,
        0x52, 0x03, 0xD7, 0xB3, 0xA2, 0xB2, 0x52, 0xB9, 0xAD, 0xA9, 0x39, 0xA3
    };

    uint8_t buf[32];
    PacketFrame frames[4];
    size_t n = buildSmsFrames("OpenRTX", 7, buf, sizeof(buf), frames, 4);
    REQUIRE(n == 1);

    FrameEncoder encoder;
    frame_t encoded;
    encoder.encodePacketFrame(frames[0], encoded);
    REQUIRE(encoded == KNOWN_GOOD_FRAME);
}
