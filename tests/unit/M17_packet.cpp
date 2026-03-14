/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <cstring>
#include <array>
#include <algorithm>
#include "protocols/M17/M17FrameEncoder.hpp"
#include "protocols/M17/M17FrameDecoder.hpp"
#include "protocols/M17/M17PacketFrame.hpp"
#include "protocols/M17/M17Datatypes.hpp"
#include "protocols/M17/M17Constants.hpp"
#include "protocols/M17/M17LinkSetupFrame.hpp"
#include "core/crc.h"

using namespace M17;

TEST_CASE("Freshly constructed PacketFrame is zeroed", "[m17][packet]")
{
    M17PacketFrame frame;
    const uint8_t *data = frame.getData();

    for (size_t i = 0; i < sizeof(pktFrame_t); i++) {
        REQUIRE(data[i] == 0);
    }
}

TEST_CASE("Payload provides read/write access", "[m17][packet]")
{
    M17PacketFrame frame;
    auto &payload = frame.payload();

    // Write a known pattern
    for (size_t i = 0; i < payload.size(); i++)
        payload[i] = static_cast<uint8_t>(i + 1);

    // Read back via getData()
    const uint8_t *data = frame.getData();
    for (size_t i = 0; i < payload.size(); i++) {
        REQUIRE(data[i] == static_cast<uint8_t>(i + 1));
    }
}

TEST_CASE("clear() resets every byte to zero", "[m17][packet]")
{
    M17PacketFrame frame;
    auto &payload = frame.payload();

    for (size_t i = 0; i < payload.size(); i++)
        payload[i] = 0xFF;
    frame.setEof(true);
    frame.setCounter(31);

    frame.clear();
    const uint8_t *data = frame.getData();
    for (size_t i = 0; i < sizeof(pktFrame_t); i++) {
        REQUIRE(data[i] == 0);
    }
}

TEST_CASE("Encode then decode round-trip", "[m17][packet]")
{
    M17FrameEncoder encoder;
    M17FrameDecoder decoder;

    // Build a test payload: "HELLO M17 PACKET!!" padded with zeros.
    M17PacketFrame frame;
    auto &payload = frame.payload();
    const char msg[] = "HELLO M17 PACKET!!";
    std::copy_n(msg, sizeof(msg) - 1, payload.begin());

    // Encode
    frame_t encoded;
    encoder.encodePacketFrame(frame, encoded);

    // The first two bytes should be the PACKET_SYNC_WORD
    REQUIRE(encoded[0] == PACKET_SYNC_WORD[0]);
    REQUIRE(encoded[1] == PACKET_SYNC_WORD[1]);

    // Decode
    M17FrameType type = decoder.decodeFrame(encoded);
    REQUIRE(type == M17FrameType::PACKET);

    const M17PacketFrame &decoded = decoder.getPacketFrame();
    const auto &decodedPayload = decoded.payload();

    REQUIRE(std::equal(payload.begin(), payload.end(), decodedPayload.begin()));
}

TEST_CASE("Round-trip with all-zeros payload", "[m17][packet]")
{
    M17FrameEncoder encoder;
    M17FrameDecoder decoder;

    M17PacketFrame frame;
    auto &payload = frame.payload();

    frame_t encoded;
    encoder.encodePacketFrame(frame, encoded);

    M17FrameType type = decoder.decodeFrame(encoded);
    REQUIRE(type == M17FrameType::PACKET);

    const M17PacketFrame &decoded = decoder.getPacketFrame();
    const auto &decodedPayload = decoded.payload();

    REQUIRE(std::equal(payload.begin(), payload.end(), decodedPayload.begin()));
}

TEST_CASE("Round-trip with sequential byte pattern", "[m17][packet]")
{
    M17FrameEncoder encoder;
    M17FrameDecoder decoder;

    M17PacketFrame frame;
    auto &payload = frame.payload();
    for (size_t i = 0; i < payload.size(); i++)
        payload[i] = static_cast<uint8_t>(i * 7 + 3); // arbitrary non-trivial

    frame_t encoded;
    encoder.encodePacketFrame(frame, encoded);

    M17FrameType type = decoder.decodeFrame(encoded);
    REQUIRE(type == M17FrameType::PACKET);

    const M17PacketFrame &decoded = decoder.getPacketFrame();
    const auto &decodedPayload = decoded.payload();

    REQUIRE(std::equal(payload.begin(), payload.end(), decodedPayload.begin()));
}

TEST_CASE("pktFrame_t is 26 bytes", "[m17][packet]")
{
    // Per M17 spec: 25 bytes of packet chunk data + 1 metadata byte = 26 bytes
    REQUIRE(sizeof(pktFrame_t) == 26);
}

TEST_CASE("Encoded frame is exactly 48 bytes with PACKET_SYNC_WORD",
          "[m17][packet]")
{
    M17FrameEncoder encoder;

    M17PacketFrame frame;
    auto &payload = frame.payload();
    payload[0] = 0xAB;

    frame_t encoded;
    encoder.encodePacketFrame(frame, encoded);

    REQUIRE(encoded.size() == 48);
    REQUIRE(encoded[0] == PACKET_SYNC_WORD[0]);
    REQUIRE(encoded[1] == PACKET_SYNC_WORD[1]);
}

TEST_CASE("Encoding the same payload twice produces identical frames",
          "[m17][packet]")
{
    M17FrameEncoder encoder;

    M17PacketFrame frame;
    auto &payload = frame.payload();
    const char msg[] = "DETERMINISTIC";
    std::copy_n(msg, sizeof(msg) - 1, payload.begin());

    frame_t encoded1, encoded2;
    encoder.encodePacketFrame(frame, encoded1);
    encoder.encodePacketFrame(frame, encoded2);

    REQUIRE(encoded1 == encoded2);
}

TEST_CASE("Round-trip with all-0xFF payload", "[m17][packet]")
{
    M17FrameEncoder encoder;
    M17FrameDecoder decoder;

    M17PacketFrame frame;
    auto &payload = frame.payload();
    payload.fill(0xFF);

    frame_t encoded;
    encoder.encodePacketFrame(frame, encoded);

    M17FrameType type = decoder.decodeFrame(encoded);
    REQUIRE(type == M17FrameType::PACKET);

    const M17PacketFrame &decoded = decoder.getPacketFrame();
    const auto &decodedPayload = decoded.payload();

    REQUIRE(std::equal(payload.begin(), payload.end(), decodedPayload.begin()));
}

TEST_CASE("Round-trip with single non-zero byte", "[m17][packet]")
{
    M17FrameEncoder encoder;
    M17FrameDecoder decoder;

    M17PacketFrame frame;
    auto &payload = frame.payload();
    payload[0] = 0x42;

    frame_t encoded;
    encoder.encodePacketFrame(frame, encoded);

    M17FrameType type = decoder.decodeFrame(encoded);
    REQUIRE(type == M17FrameType::PACKET);

    const M17PacketFrame &decoded = decoder.getPacketFrame();
    const auto &decodedPayload = decoded.payload();

    REQUIRE(std::equal(payload.begin(), payload.end(), decodedPayload.begin()));
}

TEST_CASE("Decoder distinguishes PACKET from STREAM frame type",
          "[m17][packet]")
{
    M17FrameEncoder encoder;
    M17FrameDecoder decoder;

    // Encode a packet frame
    M17PacketFrame pktFrame;
    auto &pktPayload = pktFrame.payload();
    pktPayload[0] = 0x01;
    frame_t pktEncoded;
    encoder.encodePacketFrame(pktFrame, pktEncoded);

    M17FrameType pktType = decoder.decodeFrame(pktEncoded);
    REQUIRE(pktType == M17FrameType::PACKET);

    // Encode a stream frame and verify it is NOT detected as PACKET
    payload_t streamPayload = {};
    streamPayload[0] = 0x01;
    frame_t streamEncoded;
    encoder.encodeStreamFrame(streamPayload, streamEncoded);

    M17FrameType streamType = decoder.decodeFrame(streamEncoded);
    REQUIRE(streamType == M17FrameType::STREAM);
    REQUIRE(streamType != M17FrameType::PACKET);
}

TEST_CASE("Decoder identifies EOT frame type", "[m17][packet]")
{
    M17FrameEncoder encoder;
    M17FrameDecoder decoder;

    frame_t eotFrame;
    encoder.encodeEotFrame(eotFrame);

    M17FrameType type = decoder.decodeFrame(eotFrame);
    REQUIRE(type == M17FrameType::EOT);
}

TEST_CASE("Decoder identifies LSF frame type, not PACKET", "[m17][packet]")
{
    M17FrameEncoder encoder;
    M17FrameDecoder decoder;

    M17LinkSetupFrame lsf;
    frame_t lsfEncoded;
    encoder.encodeLsf(lsf, lsfEncoded);

    M17FrameType type = decoder.decodeFrame(lsfEncoded);
    REQUIRE(type == M17FrameType::LINK_SETUP);
    REQUIRE(type != M17FrameType::PACKET);
}

TEST_CASE("Decoder reset clears previous packet frame data", "[m17][packet]")
{
    M17FrameEncoder encoder;
    M17FrameDecoder decoder;

    // Decode a non-zero payload so decoder has data
    M17PacketFrame frame;
    auto &payload = frame.payload();
    payload.fill(0xAA);
    frame_t encoded;
    encoder.encodePacketFrame(frame, encoded);
    decoder.decodeFrame(encoded);

    // Reset should clear internal state
    decoder.reset();

    const M17PacketFrame &pktFrame = decoder.getPacketFrame();
    const uint8_t *data = pktFrame.getData();

    for (size_t i = 0; i < sizeof(pktFrame_t); i++) {
        REQUIRE(data[i] == 0);
    }
}

TEST_CASE("Consecutive encodes with different payloads are independent",
          "[m17][packet]")
{
    M17FrameEncoder encoder;
    M17FrameDecoder decoder;

    M17PacketFrame frame1;
    auto &payload1 = frame1.payload();
    payload1.fill(0x11);
    M17PacketFrame frame2;
    auto &payload2 = frame2.payload();
    payload2.fill(0x22);

    frame_t encoded1, encoded2;
    encoder.encodePacketFrame(frame1, encoded1);
    encoder.encodePacketFrame(frame2, encoded2);

    // Frames should differ (different payloads)
    REQUIRE(encoded1 != encoded2);

    // Each should decode back to its own payload
    decoder.decodeFrame(encoded1);
    const auto &dec1 = decoder.getPacketFrame().payload();
    REQUIRE(std::equal(payload1.begin(), payload1.end(), dec1.begin()));

    decoder.decodeFrame(encoded2);
    const auto &dec2 = decoder.getPacketFrame().payload();
    REQUIRE(std::equal(payload2.begin(), payload2.end(), dec2.begin()));
}

TEST_CASE("setEof and isEof", "[m17][packet]")
{
    M17PacketFrame frame;

    REQUIRE(frame.isEof() == false);

    frame.setEof(true);
    REQUIRE(frame.isEof() == true);

    frame.setEof(false);
    REQUIRE(frame.isEof() == false);
}

TEST_CASE("setCounter and getCounter", "[m17][packet]")
{
    M17PacketFrame frame;

    REQUIRE(frame.getCounter() == 0);

    frame.setCounter(17);
    REQUIRE(frame.getCounter() == 17);

    frame.setCounter(31);
    REQUIRE(frame.getCounter() == 31);

    frame.setCounter(0);
    REQUIRE(frame.getCounter() == 0);
}

TEST_CASE("EOF and counter are independent", "[m17][packet]")
{
    M17PacketFrame frame;

    frame.setCounter(25);
    frame.setEof(true);
    REQUIRE(frame.getCounter() == 25);
    REQUIRE(frame.isEof() == true);

    frame.setEof(false);
    REQUIRE(frame.getCounter() == 25);
    REQUIRE(frame.isEof() == false);

    frame.setEof(true);
    frame.setCounter(10);
    REQUIRE(frame.getCounter() == 10);
    REQUIRE(frame.isEof() == true);
}

TEST_CASE("clear() resets EOF and counter", "[m17][packet]")
{
    M17PacketFrame frame;

    frame.setEof(true);
    frame.setCounter(15);
    frame.clear();

    REQUIRE(frame.isEof() == false);
    REQUIRE(frame.getCounter() == 0);
}

TEST_CASE("EOF and counter round-trip through encode/decode", "[m17][packet]")
{
    M17FrameEncoder encoder;
    M17FrameDecoder decoder;

    // Intermediate frame: counter=3, not EOF
    M17PacketFrame frame;
    auto &payload = frame.payload();
    const char msg[] = "COUNTER TEST";
    std::copy_n(msg, sizeof(msg) - 1, payload.begin());
    frame.setCounter(3);

    frame_t encoded;
    encoder.encodePacketFrame(frame, encoded);

    M17FrameType type = decoder.decodeFrame(encoded);
    REQUIRE(type == M17FrameType::PACKET);

    const M17PacketFrame &decoded = decoder.getPacketFrame();
    REQUIRE(decoded.getCounter() == 3);
    REQUIRE(decoded.isEof() == false);

    // Final frame: counter=12 (remaining bytes), EOF set
    M17PacketFrame lastFrame;
    auto &lastPayload = lastFrame.payload();
    const char msg2[] = "LAST FRAME";
    std::copy_n(msg2, sizeof(msg2) - 1, lastPayload.begin());
    lastFrame.setEof(true);
    lastFrame.setCounter(12);

    encoder.encodePacketFrame(lastFrame, encoded);
    type = decoder.decodeFrame(encoded);
    REQUIRE(type == M17FrameType::PACKET);

    const M17PacketFrame &decodedLast = decoder.getPacketFrame();
    REQUIRE(decodedLast.getCounter() == 12);
    REQUIRE(decodedLast.isEof() == true);
}

TEST_CASE("Viterbi recovers payload after single bit flip in encoded frame",
          "[m17][packet]")
{
    M17FrameEncoder encoder;
    M17FrameDecoder decoder;

    M17PacketFrame frame;
    auto &payload = frame.payload();
    const char msg[] = "HELLO M17 PACKET!!";
    std::copy_n(msg, sizeof(msg) - 1, payload.begin());

    frame_t encoded;
    encoder.encodePacketFrame(frame, encoded);

    // Flip one bit in a data byte (bytes 0-1 are the sync word, skip them)
    encoded[10] ^= 0x01;

    M17FrameType type = decoder.decodeFrame(encoded);
    REQUIRE(type == M17FrameType::PACKET);

    const M17PacketFrame &decoded = decoder.getPacketFrame();
    REQUIRE(
        std::equal(payload.begin(), payload.end(), decoded.payload().begin()));
}

TEST_CASE("Viterbi recovers payload after scattered bit flips in encoded frame",
          "[m17][packet]")
{
    M17FrameEncoder encoder;
    M17FrameDecoder decoder;

    M17PacketFrame frame;
    auto &payload = frame.payload();
    const char msg[] = "BIT ERROR TEST";
    std::copy_n(msg, sizeof(msg) - 1, payload.begin());

    frame_t encoded;
    encoder.encodePacketFrame(frame, encoded);

    // Flip three bits at scattered positions in the data portion (bytes 2-47)
    encoded[5] ^= 0x08;
    encoded[20] ^= 0x40;
    encoded[38] ^= 0x02;

    M17FrameType type = decoder.decodeFrame(encoded);
    REQUIRE(type == M17FrameType::PACKET);

    const M17PacketFrame &decoded = decoder.getPacketFrame();
    REQUIRE(
        std::equal(payload.begin(), payload.end(), decoded.payload().begin()));
}

TEST_CASE("Reference vector: assemble, encode, decode SMS packet",
          "[m17][packet]")
{
    // Ground-truth encoded frame for an SMS carrying "OpenRTX".
    // Originally captured over RF, byte 32 corrected (0xCA → 0xCB)
    // to match encoder output (single-bit air-interface error).
    static constexpr frame_t KNOWN_GOOD_FRAME = {
        0x75, 0xFF, 0xF6, 0xD4, 0xC2, 0x59, 0x82, 0x96, 0x84, 0x63, 0x9A, 0x26,
        0xF6, 0xD8, 0xB8, 0xF0, 0x9D, 0x0D, 0x4C, 0xD0, 0x00, 0x03, 0x9F, 0x05,
        0xEE, 0x6E, 0x72, 0x2F, 0x23, 0xDA, 0x96, 0xFE, 0xCB, 0x70, 0x9B, 0x09,
        0x52, 0x03, 0xD7, 0xB3, 0xA2, 0xB2, 0x52, 0xB9, 0xAD, 0xA9, 0x39, 0xA3
    };

    const char sms_text[] = "OpenRTX";

    // --- Assemble payload from first principles ---
    M17PacketFrame frame;
    auto &payload = frame.payload();
    payload[0] = 0x05;                                    // SMS protocol ID
    std::copy_n(sms_text, sizeof(sms_text), &payload[1]); // text + '\0'
    uint16_t crc = crc_m17(payload.data(), 9);
    payload[9] = static_cast<uint8_t>(crc >> 8);          // CRC high byte
    payload[10] = static_cast<uint8_t>(crc & 0xFF);       // CRC low byte
    frame.setEof(true);
    frame.setCounter(11);

    // --- Encode and compare to reference ---
    M17FrameEncoder encoder;
    frame_t encoded;
    encoder.encodePacketFrame(frame, encoded);
    REQUIRE(encoded == KNOWN_GOOD_FRAME);

    // --- Decode and verify every field ---
    M17FrameDecoder decoder;
    M17FrameType type = decoder.decodeFrame(KNOWN_GOOD_FRAME);
    REQUIRE(type == M17FrameType::PACKET);

    const M17PacketFrame &pf = decoder.getPacketFrame();
    const auto &pl = pf.payload();

    // Protocol ID
    REQUIRE(pl[0] == 0x05);

    // Message text
    for (size_t i = 0; i < sizeof(sms_text) - 1; i++)
        REQUIRE(pl[1 + i] == static_cast<uint8_t>(sms_text[i]));

    // Null terminator
    REQUIRE(pl[8] == 0x00);

    // CRC-16: verify individual bytes and validate independently
    REQUIRE(pl[9] == 0xB8);
    REQUIRE(pl[10] == 0x34);
    uint16_t decodedCrc = crc_m17(pl.data(), 9);
    REQUIRE(decodedCrc == static_cast<uint16_t>((pl[9] << 8) | pl[10]));

    // Padding
    for (size_t i = 11; i <= 24; i++)
        REQUIRE(pl[i] == 0x00);

    // Metadata byte: EOF, counter, reserved bits
    REQUIRE(pf.isEof() == true);
    REQUIRE(pf.getCounter() == 11);
    REQUIRE((pf.getData()[25] & 0x03) == 0x00);
}
