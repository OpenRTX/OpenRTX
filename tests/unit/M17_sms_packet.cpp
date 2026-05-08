/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <cstring>
#include "protocols/M17/SmsPacket.hpp"
#include "protocols/M17/PacketFramer.hpp"
#include "protocols/M17/PacketDeframer.hpp"
#include "protocols/M17/FrameEncoder.hpp"
#include "protocols/M17/FrameDecoder.hpp"
#include "protocols/M17/Datatypes.hpp"
#include "core/crc.h"

using namespace M17;

// Helper: format SMS app data and assemble into PacketFrames.
static size_t buildSmsFrames(const char *msg, size_t msgLen, uint8_t *buffer,
                             size_t bufSize, PacketFrame *frames,
                             size_t maxFrames)
{
    size_t appLen = sms_format_packet(msg, msgLen, buffer, bufSize);
    if (appLen == 0)
        return 0;

    PacketFramer framer;
    if (!framer.init(buffer, appLen))
        return 0;

    size_t n = 0;
    bool isLast = false;
    while (!isLast && n < maxFrames) {
        isLast = framer.nextFrame(frames[n]);
        n++;
    }
    return n;
}

// ===========================================================================
// Format tests (sms_format_packet)
// ===========================================================================

TEST_CASE("SMS format: empty message returns zero", "[m17][smspacket]")
{
    uint8_t buf[32];
    REQUIRE(sms_format_packet("", 0, buf, sizeof(buf)) == 0);
}

TEST_CASE("SMS format: null message pointer returns zero", "[m17][smspacket]")
{
    uint8_t buf[32];
    REQUIRE(sms_format_packet(nullptr, 5, buf, sizeof(buf)) == 0);
}

TEST_CASE("SMS format: protocol ID byte is 0x05", "[m17][smspacket]")
{
    uint8_t buf[32];
    size_t len = sms_format_packet("Hi", 2, buf, sizeof(buf));
    REQUIRE(len > 0);
    REQUIRE(buf[0] == 0x05);
}

TEST_CASE("SMS format: message text follows protocol ID", "[m17][smspacket]")
{
    uint8_t buf[32];
    sms_format_packet("AB", 2, buf, sizeof(buf));
    REQUIRE(buf[1] == 'A');
    REQUIRE(buf[2] == 'B');
}

TEST_CASE("SMS format: NUL terminator follows message", "[m17][smspacket]")
{
    uint8_t buf[32];
    sms_format_packet("AB", 2, buf, sizeof(buf));
    REQUIRE(buf[3] == 0x00);
}

TEST_CASE("SMS format: CRC is computed over protocol ID + message + NUL",
          "[m17][smspacket]")
{
    const char *msg = "Test CRC";
    size_t msgLen = strlen(msg);

    uint8_t buf[32];
    size_t appLen = sms_format_packet(msg, msgLen, buf, sizeof(buf));
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

TEST_CASE("SMS format: round-trips through encoder/decoder", "[m17][smspacket]")
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

TEST_CASE("SMS format: reference vector matches known-good frame",
          "[m17][smspacket]")
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

// ===========================================================================
// Parse tests (sms_parse_packet)
// ===========================================================================

TEST_CASE("SMS parse: null data returns false", "[m17][smspacket]")
{
    char msg[32];
    REQUIRE(sms_parse_packet(nullptr, 10, msg, sizeof(msg)) == false);
}

TEST_CASE("SMS parse: null message buffer returns false", "[m17][smspacket]")
{
    const uint8_t data[] = { 0x05, 'H', 'i', 0x00 };
    REQUIRE(sms_parse_packet(data, sizeof(data), nullptr, 0) == false);
}

TEST_CASE("SMS parse: too short returns false", "[m17][smspacket]")
{
    const uint8_t data[] = { 0x05 };
    char msg[32];
    REQUIRE(sms_parse_packet(data, 1, msg, sizeof(msg)) == false);
}

TEST_CASE("SMS parse: wrong protocol ID returns false", "[m17][smspacket]")
{
    const uint8_t data[] = { 0x04, 'H', 'i', 0x00 };
    char msg[32];
    REQUIRE(sms_parse_packet(data, sizeof(data), msg, sizeof(msg)) == false);
}

TEST_CASE("SMS parse: extracts message text", "[m17][smspacket]")
{
    const uint8_t data[] = { 0x05, 'H', 'e', 'l', 'l', 'o', 0x00 };
    char msg[32];
    bool ok = sms_parse_packet(data, sizeof(data), msg, sizeof(msg));
    REQUIRE(ok == true);
    REQUIRE(strcmp(msg, "Hello") == 0);
}

TEST_CASE("SMS parse: works without trailing NUL in payload",
          "[m17][smspacket]")
{
    const uint8_t data[] = { 0x05, 'H', 'i' };
    char msg[32];
    bool ok = sms_parse_packet(data, sizeof(data), msg, sizeof(msg));
    REQUIRE(ok == true);
    REQUIRE(strcmp(msg, "Hi") == 0);
}

TEST_CASE("SMS parse: truncates to msgSize - 1", "[m17][smspacket]")
{
    const uint8_t data[] = { 0x05, 'A', 'B', 'C', 'D', 0x00 };
    char msg[4]; // room for 3 chars + NUL
    bool ok = sms_parse_packet(data, sizeof(data), msg, sizeof(msg));
    REQUIRE(ok == true);
    REQUIRE(msg[3] == '\0');
    REQUIRE(strlen(msg) == 3);
}

// ---------- Round-trip: format then parse ----------

TEST_CASE("SMS round-trip: format then parse recovers original message",
          "[m17][smspacket]")
{
    const char *original = "Hello, OpenRTX!";
    size_t origLen = strlen(original);

    // Format
    uint8_t buf[64];
    size_t appLen = sms_format_packet(original, origLen, buf, sizeof(buf));
    REQUIRE(appLen == 1 + origLen + 1);

    // Parse
    char recovered[64];
    bool ok = sms_parse_packet(buf, appLen, recovered, sizeof(recovered));
    REQUIRE(ok == true);
    REQUIRE(strcmp(recovered, original) == 0);
}

// ---------- Full pipeline: format -> frame -> deframe -> parse ----------

TEST_CASE("SMS pipeline: format/frame/deframe/parse round-trip recovers message",
          "[m17][smspacket]")
{
    const char *original = "Testing 1 2 3";
    size_t origLen = strlen(original);

    // Format
    uint8_t buf[64];
    size_t appLen = sms_format_packet(original, origLen, buf, sizeof(buf));
    REQUIRE(appLen > 0);

    // Frame
    PacketFramer framer;
    REQUIRE(framer.init(buf, appLen) == true);

    // Deframe
    uint8_t rxBuf[64];
    PacketDeframer deframer;
    REQUIRE(deframer.init(rxBuf, sizeof(rxBuf)) == true);
    PacketFrame frame;
    DeframerResult result = DeframerResult::IN_PROGRESS;
    bool isLast = false;
    while (!isLast) {
        isLast = framer.nextFrame(frame);
        result = deframer.pushFrame(frame);
    }
    REQUIRE(result == DeframerResult::COMPLETE);

    // Parse
    char recovered[64];
    bool ok = sms_parse_packet(deframer.data(), deframer.length(), recovered,
                               sizeof(recovered));
    REQUIRE(ok == true);
    REQUIRE(strcmp(recovered, original) == 0);
}
