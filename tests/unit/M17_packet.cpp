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
#include "protocols/M17/FrameEncoder.hpp"
#include "protocols/M17/FrameDecoder.hpp"
#include "protocols/M17/PacketFrame.hpp"
#include "protocols/M17/Datatypes.hpp"
#include "protocols/M17/Constants.hpp"
#include "protocols/M17/LinkSetupFrame.hpp"
#include "core/crc.h"

using namespace M17;

TEST_CASE("Freshly constructed PacketFrame is zeroed", "[m17][packet]")
{
    PacketFrame frame;
    const uint8_t *raw = frame.data();

    for (size_t i = 0; i < PacketFrame::FRAME_SIZE; i++) {
        REQUIRE(raw[i] == 0);
    }
}

TEST_CASE("Payload provides read/write access", "[m17][packet]")
{
    PacketFrame frame;

    // Write a known pattern
    for (size_t i = 0; i < PacketFrame::DATA_SIZE; i++)
        frame[i] = static_cast<uint8_t>(i + 1);

    // Read back via data()
    const uint8_t *raw = frame.data();
    for (size_t i = 0; i < PacketFrame::DATA_SIZE; i++) {
        REQUIRE(raw[i] == static_cast<uint8_t>(i + 1));
    }
}

TEST_CASE("clear() resets every byte to zero", "[m17][packet]")
{
    PacketFrame frame;

    for (size_t i = 0; i < PacketFrame::DATA_SIZE; i++)
        frame[i] = 0xFF;
    frame.setEof(true);
    frame.setCounter(31);

    frame.clear();
    const uint8_t *raw = frame.data();
    for (size_t i = 0; i < PacketFrame::FRAME_SIZE; i++) {
        REQUIRE(raw[i] == 0);
    }
}

TEST_CASE("Encode then decode round-trip", "[m17][packet]")
{
    FrameEncoder encoder;
    FrameDecoder decoder;

    // Build a test payload: "HELLO M17 PACKET!!" padded with zeros.
    PacketFrame frame;
    const char msg[] = "HELLO M17 PACKET!!";
    std::copy_n(msg, sizeof(msg) - 1, &frame[0]);

    // Encode
    frame_t encoded;
    encoder.encodePacketFrame(frame, encoded);

    // The first two bytes should be the PACKET_SYNC_WORD
    REQUIRE(encoded[0] == PACKET_SYNC_WORD[0]);
    REQUIRE(encoded[1] == PACKET_SYNC_WORD[1]);

    // Decode
    FrameType type = decoder.decodeFrame(encoded);
    REQUIRE(type == FrameType::PACKET);

    const PacketFrame &decoded = decoder.getPacketFrame();

    REQUIRE(memcmp(frame.data(), decoded.data(), PacketFrame::DATA_SIZE) == 0);
}

TEST_CASE("Round-trip with all-zeros payload", "[m17][packet]")
{
    FrameEncoder encoder;
    FrameDecoder decoder;

    PacketFrame frame;

    frame_t encoded;
    encoder.encodePacketFrame(frame, encoded);

    FrameType type = decoder.decodeFrame(encoded);
    REQUIRE(type == FrameType::PACKET);

    const PacketFrame &decoded = decoder.getPacketFrame();

    REQUIRE(memcmp(frame.data(), decoded.data(), PacketFrame::DATA_SIZE) == 0);
}

TEST_CASE("Round-trip with sequential byte pattern", "[m17][packet]")
{
    FrameEncoder encoder;
    FrameDecoder decoder;

    PacketFrame frame;
    for (size_t i = 0; i < PacketFrame::DATA_SIZE; i++)
        frame[i] = static_cast<uint8_t>(i * 7 + 3); // arbitrary non-trivial

    frame_t encoded;
    encoder.encodePacketFrame(frame, encoded);

    FrameType type = decoder.decodeFrame(encoded);
    REQUIRE(type == FrameType::PACKET);

    const PacketFrame &decoded = decoder.getPacketFrame();

    REQUIRE(memcmp(frame.data(), decoded.data(), PacketFrame::DATA_SIZE) == 0);
}

TEST_CASE("PacketFrame FRAME_SIZE is 26 bytes", "[m17][packet]")
{
    // Per M17 spec: 25 bytes of packet chunk data + 1 metadata byte = 26 bytes
    REQUIRE(PacketFrame::FRAME_SIZE == 26);
}

TEST_CASE("Encoded frame is exactly 48 bytes with PACKET_SYNC_WORD",
          "[m17][packet]")
{
    FrameEncoder encoder;

    PacketFrame frame;
    frame[0] = 0xAB;

    frame_t encoded;
    encoder.encodePacketFrame(frame, encoded);

    REQUIRE(encoded.size() == 48);
    REQUIRE(encoded[0] == PACKET_SYNC_WORD[0]);
    REQUIRE(encoded[1] == PACKET_SYNC_WORD[1]);
}

TEST_CASE("Encoding the same payload twice produces identical frames",
          "[m17][packet]")
{
    FrameEncoder encoder;

    PacketFrame frame;
    const char msg[] = "DETERMINISTIC";
    std::copy_n(msg, sizeof(msg) - 1, &frame[0]);

    frame_t encoded1, encoded2;
    encoder.encodePacketFrame(frame, encoded1);
    encoder.encodePacketFrame(frame, encoded2);

    REQUIRE(encoded1 == encoded2);
}

TEST_CASE("Round-trip with all-0xFF payload", "[m17][packet]")
{
    FrameEncoder encoder;
    FrameDecoder decoder;

    PacketFrame frame;
    for (size_t i = 0; i < PacketFrame::DATA_SIZE; i++)
        frame[i] = 0xFF;

    frame_t encoded;
    encoder.encodePacketFrame(frame, encoded);

    FrameType type = decoder.decodeFrame(encoded);
    REQUIRE(type == FrameType::PACKET);

    const PacketFrame &decoded = decoder.getPacketFrame();

    REQUIRE(memcmp(frame.data(), decoded.data(), PacketFrame::DATA_SIZE) == 0);
}

TEST_CASE("Round-trip with single non-zero byte", "[m17][packet]")
{
    FrameEncoder encoder;
    FrameDecoder decoder;

    PacketFrame frame;
    frame[0] = 0x42;

    frame_t encoded;
    encoder.encodePacketFrame(frame, encoded);

    FrameType type = decoder.decodeFrame(encoded);
    REQUIRE(type == FrameType::PACKET);

    const PacketFrame &decoded = decoder.getPacketFrame();

    REQUIRE(memcmp(frame.data(), decoded.data(), PacketFrame::DATA_SIZE) == 0);
}

TEST_CASE("Decoder distinguishes PACKET from STREAM frame type",
          "[m17][packet]")
{
    FrameEncoder encoder;
    FrameDecoder decoder;

    // Encode a packet frame
    PacketFrame pktFrame;
    pktFrame[0] = 0x01;
    frame_t pktEncoded;
    encoder.encodePacketFrame(pktFrame, pktEncoded);

    FrameType pktType = decoder.decodeFrame(pktEncoded);
    REQUIRE(pktType == FrameType::PACKET);

    // Encode a stream frame and verify it is NOT detected as PACKET
    payload_t streamPayload = {};
    streamPayload[0] = 0x01;
    frame_t streamEncoded;
    encoder.encodeStreamFrame(streamPayload, streamEncoded);

    FrameType streamType = decoder.decodeFrame(streamEncoded);
    REQUIRE(streamType == FrameType::STREAM);
    REQUIRE(streamType != FrameType::PACKET);
}

TEST_CASE("Decoder identifies EOT frame type", "[m17][packet]")
{
    FrameEncoder encoder;
    FrameDecoder decoder;

    frame_t eotFrame;
    encoder.encodeEotFrame(eotFrame);

    FrameType type = decoder.decodeFrame(eotFrame);
    REQUIRE(type == FrameType::EOT);
}

TEST_CASE("Decoder identifies LSF frame type, not PACKET", "[m17][packet]")
{
    FrameEncoder encoder;
    FrameDecoder decoder;

    LinkSetupFrame lsf;
    frame_t lsfEncoded;
    encoder.encodeLsf(lsf, lsfEncoded);

    FrameType type = decoder.decodeFrame(lsfEncoded);
    REQUIRE(type == FrameType::LINK_SETUP);
    REQUIRE(type != FrameType::PACKET);
}

TEST_CASE("Decoder reset clears previous packet frame data", "[m17][packet]")
{
    FrameEncoder encoder;
    FrameDecoder decoder;

    // Decode a non-zero payload so decoder has data
    PacketFrame frame;
    for (size_t i = 0; i < PacketFrame::DATA_SIZE; i++)
        frame[i] = 0xAA;
    frame_t encoded;
    encoder.encodePacketFrame(frame, encoded);
    decoder.decodeFrame(encoded);

    // Reset should clear internal state
    decoder.reset();

    const PacketFrame &pktFrame = decoder.getPacketFrame();
    const uint8_t *raw = pktFrame.data();

    for (size_t i = 0; i < PacketFrame::FRAME_SIZE; i++) {
        REQUIRE(raw[i] == 0);
    }
}

TEST_CASE("Consecutive encodes with different payloads are independent",
          "[m17][packet]")
{
    FrameEncoder encoder;
    FrameDecoder decoder;

    PacketFrame frame1;
    for (size_t i = 0; i < PacketFrame::DATA_SIZE; i++)
        frame1[i] = 0x11;
    PacketFrame frame2;
    for (size_t i = 0; i < PacketFrame::DATA_SIZE; i++)
        frame2[i] = 0x22;

    frame_t encoded1, encoded2;
    encoder.encodePacketFrame(frame1, encoded1);
    encoder.encodePacketFrame(frame2, encoded2);

    // Frames should differ (different payloads)
    REQUIRE(encoded1 != encoded2);

    // Each should decode back to its own payload
    decoder.decodeFrame(encoded1);
    const PacketFrame &dec1 = decoder.getPacketFrame();
    REQUIRE(memcmp(frame1.data(), dec1.data(), PacketFrame::DATA_SIZE) == 0);

    decoder.decodeFrame(encoded2);
    const PacketFrame &dec2 = decoder.getPacketFrame();
    REQUIRE(memcmp(frame2.data(), dec2.data(), PacketFrame::DATA_SIZE) == 0);
}

TEST_CASE("setEof and isEof", "[m17][packet]")
{
    PacketFrame frame;

    REQUIRE(frame.isEof() == false);

    frame.setEof(true);
    REQUIRE(frame.isEof() == true);

    frame.setEof(false);
    REQUIRE(frame.isEof() == false);
}

TEST_CASE("setCounter and getCounter", "[m17][packet]")
{
    PacketFrame frame;

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
    PacketFrame frame;

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
    PacketFrame frame;

    frame.setEof(true);
    frame.setCounter(15);
    frame.clear();

    REQUIRE(frame.isEof() == false);
    REQUIRE(frame.getCounter() == 0);
}

TEST_CASE("EOF and counter round-trip through encode/decode", "[m17][packet]")
{
    FrameEncoder encoder;
    FrameDecoder decoder;

    // Intermediate frame: counter=3, not EOF
    PacketFrame frame;
    const char msg[] = "COUNTER TEST";
    std::copy_n(msg, sizeof(msg) - 1, &frame[0]);
    frame.setCounter(3);

    frame_t encoded;
    encoder.encodePacketFrame(frame, encoded);

    FrameType type = decoder.decodeFrame(encoded);
    REQUIRE(type == FrameType::PACKET);

    const PacketFrame &decoded = decoder.getPacketFrame();
    REQUIRE(decoded.getCounter() == 3);
    REQUIRE(decoded.isEof() == false);

    // Final frame: counter=12 (remaining bytes), EOF set
    PacketFrame lastFrame;
    const char msg2[] = "LAST FRAME";
    std::copy_n(msg2, sizeof(msg2) - 1, &lastFrame[0]);
    lastFrame.setEof(true);
    lastFrame.setCounter(12);

    encoder.encodePacketFrame(lastFrame, encoded);
    type = decoder.decodeFrame(encoded);
    REQUIRE(type == FrameType::PACKET);

    const PacketFrame &decodedLast = decoder.getPacketFrame();
    REQUIRE(decodedLast.getCounter() == 12);
    REQUIRE(decodedLast.isEof() == true);
}

TEST_CASE("Viterbi recovers payload after single bit flip in encoded frame",
          "[m17][packet]")
{
    FrameEncoder encoder;
    FrameDecoder decoder;

    PacketFrame frame;
    const char msg[] = "HELLO M17 PACKET!!";
    std::copy_n(msg, sizeof(msg) - 1, &frame[0]);

    frame_t encoded;
    encoder.encodePacketFrame(frame, encoded);

    // Flip one bit in a data byte (bytes 0-1 are the sync word, skip them)
    encoded[10] ^= 0x01;

    FrameType type = decoder.decodeFrame(encoded);
    REQUIRE(type == FrameType::PACKET);

    const PacketFrame &decoded = decoder.getPacketFrame();
    REQUIRE(memcmp(frame.data(), decoded.data(), PacketFrame::DATA_SIZE) == 0);
}

TEST_CASE("Viterbi recovers payload after scattered bit flips in encoded frame",
          "[m17][packet]")
{
    FrameEncoder encoder;
    FrameDecoder decoder;

    PacketFrame frame;
    const char msg[] = "BIT ERROR TEST";
    std::copy_n(msg, sizeof(msg) - 1, &frame[0]);

    frame_t encoded;
    encoder.encodePacketFrame(frame, encoded);

    // Flip three bits at scattered positions in the data portion (bytes 2-47)
    encoded[5] ^= 0x08;
    encoded[20] ^= 0x40;
    encoded[38] ^= 0x02;

    FrameType type = decoder.decodeFrame(encoded);
    REQUIRE(type == FrameType::PACKET);

    const PacketFrame &decoded = decoder.getPacketFrame();
    REQUIRE(memcmp(frame.data(), decoded.data(), PacketFrame::DATA_SIZE) == 0);
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
    PacketFrame frame;
    frame[0] = 0x05;                                    // SMS protocol ID
    std::copy_n(sms_text, sizeof(sms_text), &frame[1]); // text + '\0'
    uint16_t crc = crc_m17(frame.data(), 9);
    frame[9] = static_cast<uint8_t>(crc >> 8);          // CRC high byte
    frame[10] = static_cast<uint8_t>(crc & 0xFF);       // CRC low byte
    frame.setEof(true);
    frame.setCounter(11);

    // --- Encode and compare to reference ---
    FrameEncoder encoder;
    frame_t encoded;
    encoder.encodePacketFrame(frame, encoded);
    REQUIRE(encoded == KNOWN_GOOD_FRAME);

    // --- Decode and verify every field ---
    FrameDecoder decoder;
    FrameType type = decoder.decodeFrame(KNOWN_GOOD_FRAME);
    REQUIRE(type == FrameType::PACKET);

    const PacketFrame &pf = decoder.getPacketFrame();

    // Protocol ID
    REQUIRE(pf[0] == 0x05);

    // Message text
    for (size_t i = 0; i < sizeof(sms_text) - 1; i++)
        REQUIRE(pf[1 + i] == static_cast<uint8_t>(sms_text[i]));

    // Null terminator
    REQUIRE(pf[8] == 0x00);

    // CRC-16: verify individual bytes and validate independently
    REQUIRE(pf[9] == 0xB8);
    REQUIRE(pf[10] == 0x34);
    uint16_t decodedCrc = crc_m17(pf.data(), 9);
    REQUIRE(decodedCrc == static_cast<uint16_t>((pf[9] << 8) | pf[10]));

    // Padding
    for (size_t i = 11; i <= 24; i++)
        REQUIRE(pf[i] == 0x00);

    // Metadata byte: EOF, counter, reserved bits
    REQUIRE(pf.isEof() == true);
    REQUIRE(pf.getCounter() == 11);
    REQUIRE((pf.data()[25] & 0x03) == 0x00);
}

// ===========================================================================
// SMS-specific PacketFrame tests
// ===========================================================================

TEST_CASE(
    "SMS-typed PacketFrame round-trip preserves 0x05 application-layer type byte",
    "[m17][packet][sms]")
{
    FrameEncoder encoder;
    FrameDecoder decoder;

    PacketFrame frame;
    frame[0] = 0x05; // SMS type byte
    const char payload[] = "SMS content";
    std::copy_n(payload, sizeof(payload) - 1, &frame[1]);

    frame_t encoded;
    encoder.encodePacketFrame(frame, encoded);

    FrameType type = decoder.decodeFrame(encoded);
    REQUIRE(type == FrameType::PACKET);

    const PacketFrame &decoded = decoder.getPacketFrame();
    REQUIRE(decoded[0] == 0x05);
    for (size_t i = 0; i < sizeof(payload) - 1; i++)
        REQUIRE(decoded[1 + i] == static_cast<uint8_t>(payload[i]));
}

TEST_CASE(
    "Multi-frame SMS packet encode/decode sequence preserves payload and metadata",
    "[m17][packet][sms]")
{
    FrameEncoder encoder;
    FrameDecoder decoder;

    // Build three simulated SMS frames: two intermediate + one EOF.
    // Payload: 25 bytes of sequential data per intermediate frame.
    constexpr size_t NUM_FRAMES = 3;

    PacketFrame txFrames[NUM_FRAMES];

    // Frame 0: intermediate, counter = 0
    for (size_t i = 0; i < PacketFrame::DATA_SIZE; i++)
        txFrames[0][i] = i;
    txFrames[0].setEof(false);
    txFrames[0].setCounter(0);

    // Frame 1: intermediate, counter = 1
    for (size_t i = 0; i < PacketFrame::DATA_SIZE; i++)
        txFrames[1][i] = i + PacketFrame::DATA_SIZE;
    txFrames[1].setEof(false);
    txFrames[1].setCounter(1);

    // Frame 2: EOF, counter = number of valid bytes in last chunk (12)
    constexpr uint8_t LAST_BYTES = 12;
    for (size_t i = 0; i < LAST_BYTES; i++)
        txFrames[2][i] = i + 2 * PacketFrame::DATA_SIZE;
    txFrames[2].setEof(true);
    txFrames[2].setCounter(LAST_BYTES);

    // Encode then decode each frame independently and verify round-trip.
    for (size_t f = 0; f < NUM_FRAMES; f++) {
        frame_t encoded;
        encoder.encodePacketFrame(txFrames[f], encoded);

        REQUIRE(encoded[0] == PACKET_SYNC_WORD[0]);
        REQUIRE(encoded[1] == PACKET_SYNC_WORD[1]);

        FrameType type = decoder.decodeFrame(encoded);
        REQUIRE(type == FrameType::PACKET);

        const PacketFrame &rx = decoder.getPacketFrame();

        // Payload bytes
        REQUIRE(memcmp(txFrames[f].data(), rx.data(), PacketFrame::DATA_SIZE)
                == 0);

        // Metadata
        REQUIRE(rx.isEof() == txFrames[f].isEof());
        REQUIRE(rx.getCounter() == txFrames[f].getCounter());
    }

    // Extra checks for the last (EOF) frame.
    {
        frame_t encoded;
        encoder.encodePacketFrame(txFrames[NUM_FRAMES - 1], encoded);
        decoder.decodeFrame(encoded);
        const PacketFrame &rx = decoder.getPacketFrame();

        REQUIRE(rx.isEof() == true);
        REQUIRE(rx.getCounter() == LAST_BYTES);
    }
}
