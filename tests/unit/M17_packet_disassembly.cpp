/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <cstring>
#include "protocols/M17/PacketDeframer.hpp"
#include "protocols/M17/PacketFramer.hpp"
#include "core/crc.h"

using namespace M17;

// Helper: run the framer and collect all frames into an array.
static size_t frameAll(uint8_t *data, size_t dataLen, PacketFrame *frames,
                       size_t maxFrames)
{
    PacketFramer framer;
    if (!framer.init(data, dataLen))
        return 0;

    size_t n = 0;
    bool isLast = false;
    while (!isLast && n < maxFrames) {
        isLast = framer.nextFrame(frames[n]);
        n++;
    }
    return n;
}

// Helper: init a deframer backed by a shared static buffer.
static uint8_t s_rxBuf[M17_MAX_PACKET_DATA];
static PacketDeframer makeDeframer()
{
    PacketDeframer d;
    d.init(s_rxBuf, sizeof(s_rxBuf));
    return d;
}

// ==========================================================================
// init() API tests
// ==========================================================================

TEST_CASE("PacketDeframer: init with null buffer returns false",
          "[m17][deframer]")
{
    PacketDeframer d;
    REQUIRE(d.init(nullptr, 64) == false);
}

TEST_CASE("PacketDeframer: init with zero capacity returns false",
          "[m17][deframer]")
{
    uint8_t buf[64];
    PacketDeframer d;
    REQUIRE(d.init(buf, 0) == false);
}

TEST_CASE("PacketDeframer: pushFrame before init returns ERR_OVERFLOW",
          "[m17][deframer]")
{
    PacketDeframer d; // no init

    uint8_t appData[8] = { 0x05, 'A' };
    PacketFrame frames[4];
    size_t n = frameAll(appData, 2, frames, 4);
    REQUIRE(n == 1);

    auto r = d.pushFrame(frames[0]);
    REQUIRE(r == DeframerResult::ERR_OVERFLOW);
}

// ==========================================================================
// Reset / initial state
// ==========================================================================

TEST_CASE("PacketDeframer: fresh instance has zero length", "[m17][deframer]")
{
    PacketDeframer d;
    REQUIRE(d.length() == 0);
}

TEST_CASE("PacketDeframer: reset clears state for reuse", "[m17][deframer]")
{
    uint8_t rxBuf[M17_MAX_PACKET_DATA];
    PacketDeframer d;
    REQUIRE(d.init(rxBuf, sizeof(rxBuf)) == true);

    uint8_t appData[8] = { 0x05, 'A' };
    PacketFrame frames[4];
    size_t n = frameAll(appData, 2, frames, 4);
    REQUIRE(n == 1);

    auto r = d.pushFrame(frames[0]);
    REQUIRE(r == DeframerResult::COMPLETE);
    REQUIRE(d.length() == 2);

    d.reset();
    REQUIRE(d.length() == 0);

    uint8_t appData2[8] = { 0x05, 'B', 'C' };
    PacketFrame frames2[4];
    n = frameAll(appData2, 3, frames2, 4);
    REQUIRE(n == 1);

    r = d.pushFrame(frames2[0]);
    REQUIRE(r == DeframerResult::COMPLETE);
    REQUIRE(d.length() == 3);
    REQUIRE(memcmp(d.data(), appData2, 3) == 0);
}

// ==========================================================================
// Single-frame packets
// ==========================================================================

TEST_CASE("PacketDeframer: single-frame packet completes with valid CRC",
          "[m17][deframer]")
{
    auto d = makeDeframer();
    uint8_t appData[8] = { 0x05, 'H', 'i' };
    PacketFrame frames[4];
    size_t n = frameAll(appData, 3, frames, 4);
    REQUIRE(n == 1);

    auto r = d.pushFrame(frames[0]);
    REQUIRE(r == DeframerResult::COMPLETE);
    REQUIRE(d.length() == 3);
    REQUIRE(memcmp(d.data(), appData, 3) == 0);
}

TEST_CASE("PacketDeframer: single-frame preserves all data bytes",
          "[m17][deframer]")
{
    auto d = makeDeframer();

    uint8_t appData[16];
    for (size_t i = 0; i < 11; i++)
        appData[i] = i;

    PacketFrame frames[4];
    size_t n = frameAll(appData, 11, frames, 4);
    REQUIRE(n == 1);

    auto r = d.pushFrame(frames[0]);
    REQUIRE(r == DeframerResult::COMPLETE);

    for (size_t i = 0; i < 11; i++)
        REQUIRE(d.data()[i] == appData[i]);
}

// ==========================================================================
// Multi-frame packets
// ==========================================================================

TEST_CASE("PacketDeframer: two-frame packet reassembles correctly",
          "[m17][deframer]")
{
    auto d = makeDeframer();

    uint8_t appData[32];
    for (size_t i = 0; i < 26; i++)
        appData[i] = i;

    PacketFrame frames[4];
    size_t n = frameAll(appData, 26, frames, 4);
    REQUIRE(n == 2);

    REQUIRE(d.pushFrame(frames[0]) == DeframerResult::IN_PROGRESS);
    REQUIRE(d.pushFrame(frames[1]) == DeframerResult::COMPLETE);
    REQUIRE(d.length() == 26);
    REQUIRE(memcmp(d.data(), appData, 26) == 0);
}

TEST_CASE("PacketDeframer: three-frame packet with sequential counters",
          "[m17][deframer]")
{
    auto d = makeDeframer();

    uint8_t appData[64];
    for (size_t i = 0; i < 55; i++)
        appData[i] = i & 0xFF;

    PacketFrame frames[4];
    size_t n = frameAll(appData, 55, frames, 4);
    REQUIRE(n == 3);

    REQUIRE(d.pushFrame(frames[0]) == DeframerResult::IN_PROGRESS);
    REQUIRE(d.pushFrame(frames[1]) == DeframerResult::IN_PROGRESS);
    REQUIRE(d.pushFrame(frames[2]) == DeframerResult::COMPLETE);
    REQUIRE(d.length() == 55);
    REQUIRE(memcmp(d.data(), appData, 55) == 0);
}

// ==========================================================================
// Error cases: sequence errors
// ==========================================================================

TEST_CASE("PacketDeframer: frame gap returns ERR_SEQUENCE", "[m17][deframer]")
{
    auto d = makeDeframer();

    uint8_t appData[32];
    memset(appData, 0xAA, 26);
    PacketFrame frames[4];
    size_t n = frameAll(appData, 26, frames, 4);
    REQUIRE(n == 2);

    PacketFrame wrongFrame;
    memcpy(wrongFrame.data(), frames[0].data(), PacketFrame::DATA_SIZE);
    wrongFrame.setCounter(1);
    wrongFrame.setEof(false);

    REQUIRE(d.pushFrame(wrongFrame) == DeframerResult::ERR_SEQUENCE);
}

TEST_CASE("PacketDeframer: duplicate counter returns ERR_SEQUENCE",
          "[m17][deframer]")
{
    auto d = makeDeframer();

    uint8_t appData[64];
    memset(appData, 0xBB, 55);
    PacketFrame frames[4];
    size_t n = frameAll(appData, 55, frames, 4);
    REQUIRE(n == 3);

    REQUIRE(d.pushFrame(frames[0]) == DeframerResult::IN_PROGRESS);
    REQUIRE(d.pushFrame(frames[0]) == DeframerResult::ERR_SEQUENCE);
}

// ==========================================================================
// Error cases: invalid EOF counter
// ==========================================================================

TEST_CASE("PacketDeframer: EOF counter of zero returns ERR_INVALID_FRAME",
          "[m17][deframer]")
{
    auto d = makeDeframer();

    PacketFrame bad;
    bad.setEof(true);
    bad.setCounter(0);

    REQUIRE(d.pushFrame(bad) == DeframerResult::ERR_INVALID_FRAME);
}

TEST_CASE("PacketDeframer: EOF counter > DATA_SIZE returns ERR_INVALID_FRAME",
          "[m17][deframer]")
{
    auto d = makeDeframer();

    PacketFrame bad;
    bad.setEof(true);
    bad.setCounter(PacketFrame::DATA_SIZE + 1);

    REQUIRE(d.pushFrame(bad) == DeframerResult::ERR_INVALID_FRAME);
}

// ==========================================================================
// Error cases: CRC corruption
// ==========================================================================

TEST_CASE("PacketDeframer: corrupted CRC returns ERR_CRC", "[m17][deframer]")
{
    auto d = makeDeframer();

    uint8_t appData[8] = { 0x05, 'T', 'e', 's', 't' };
    PacketFrame frames[4];
    size_t n = frameAll(appData, 5, frames, 4);
    REQUIRE(n == 1);

    frames[0][5] ^= 0xFF;

    REQUIRE(d.pushFrame(frames[0]) == DeframerResult::ERR_CRC);
}

TEST_CASE("PacketDeframer: corrupted payload data fails CRC", "[m17][deframer]")
{
    auto d = makeDeframer();

    uint8_t appData[8] = { 0x05, 'H', 'e', 'l', 'l', 'o' };
    PacketFrame frames[4];
    size_t n = frameAll(appData, 6, frames, 4);
    REQUIRE(n == 1);

    frames[0][2] ^= 0x01;

    REQUIRE(d.pushFrame(frames[0]) == DeframerResult::ERR_CRC);
}

// ==========================================================================
// Binary data with embedded nulls
// ==========================================================================

TEST_CASE("PacketDeframer: binary data with embedded NULs", "[m17][deframer]")
{
    auto d = makeDeframer();

    uint8_t appData[8] = { 0x00, 0x00, 0xFF, 0x00, 0xAB, 0x00 };
    PacketFrame frames[4];
    size_t n = frameAll(appData, 6, frames, 4);
    REQUIRE(n == 1);

    REQUIRE(d.pushFrame(frames[0]) == DeframerResult::COMPLETE);
    REQUIRE(d.length() == 6);
    REQUIRE(memcmp(d.data(), appData, 6) == 0);
}

// ==========================================================================
// Exact frame boundary (25 bytes data -> 27 with CRC -> 2 frames)
// ==========================================================================

TEST_CASE("PacketDeframer: data at exact 25-byte boundary", "[m17][deframer]")
{
    auto d = makeDeframer();

    uint8_t appData[32];
    for (size_t i = 0; i < 25; i++)
        appData[i] = i + 1;

    PacketFrame frames[4];
    size_t n = frameAll(appData, 25, frames, 4);
    REQUIRE(n == 2);

    REQUIRE(d.pushFrame(frames[0]) == DeframerResult::IN_PROGRESS);
    REQUIRE(d.pushFrame(frames[1]) == DeframerResult::COMPLETE);
    REQUIRE(d.length() == 25);
    REQUIRE(memcmp(d.data(), appData, 25) == 0);
}

// ==========================================================================
// TX -> RX round-trip integration tests
// ==========================================================================

TEST_CASE("Round-trip: single-frame SMS through PacketFramer + PacketDeframer",
          "[m17][integration]")
{
    uint8_t appData[16] = { 0x05, 'O', 'p', 'e', 'n', 'R', 'T', 'X', 0x00 };
    PacketFrame frames[4];
    size_t n = frameAll(appData, 9, frames, 4);
    REQUIRE(n > 0);

    auto d = makeDeframer();
    DeframerResult r = DeframerResult::IN_PROGRESS;
    for (size_t i = 0; i < n; i++)
        r = d.pushFrame(frames[i]);

    REQUIRE(r == DeframerResult::COMPLETE);
    REQUIRE(d.length() == 9);
    REQUIRE(memcmp(d.data(), appData, 9) == 0);
    REQUIRE(d.data()[0] == 0x05);
    REQUIRE(strcmp(reinterpret_cast<const char *>(&d.data()[1]), "OpenRTX")
            == 0);
}

TEST_CASE("Round-trip: multi-frame packet through PacketFramer + PacketDeframer",
          "[m17][integration]")
{
    uint8_t appData[104];
    appData[0] = 0x05;
    for (size_t i = 1; i < 100; i++)
        appData[i] = i & 0xFF;

    PacketFrame frames[8];
    size_t n = frameAll(appData, 100, frames, 8);
    REQUIRE(n > 1);

    auto d = makeDeframer();
    DeframerResult r = DeframerResult::IN_PROGRESS;
    for (size_t i = 0; i < n; i++) {
        r = d.pushFrame(frames[i]);
        if (i < n - 1)
            REQUIRE(r == DeframerResult::IN_PROGRESS);
    }

    REQUIRE(r == DeframerResult::COMPLETE);
    REQUIRE(d.length() == 100);
    REQUIRE(memcmp(d.data(), appData, 100) == 0);
}

TEST_CASE("Round-trip: raw protocol ID 0x00 (non-SMS) round-trips correctly",
          "[m17][integration]")
{
    uint8_t appData[8] = { 0x00, 0xDE, 0xAD, 0xBE, 0xEF };
    PacketFrame frames[4];
    size_t n = frameAll(appData, 5, frames, 4);
    REQUIRE(n == 1);

    auto d = makeDeframer();
    REQUIRE(d.pushFrame(frames[0]) == DeframerResult::COMPLETE);
    REQUIRE(d.length() == 5);
    REQUIRE(memcmp(d.data(), appData, 5) == 0);
}

TEST_CASE("Round-trip: max-size packet (823 bytes app data)",
          "[m17][integration]")
{
    uint8_t appData[825];
    for (size_t i = 0; i < 823; i++)
        appData[i] = i & 0xFF;
    appData[0] = 0x05;

    PacketFrame frames[33];
    size_t n = frameAll(appData, 823, frames, 33);
    REQUIRE(n == 33);

    uint8_t rxBuf[M17_MAX_PACKET_DATA];
    PacketDeframer d;
    REQUIRE(d.init(rxBuf, sizeof(rxBuf)) == true);

    DeframerResult r = DeframerResult::IN_PROGRESS;
    for (size_t i = 0; i < n; i++)
        r = d.pushFrame(frames[i]);

    REQUIRE(r == DeframerResult::COMPLETE);
    REQUIRE(d.length() == 823);
    REQUIRE(memcmp(d.data(), appData, 823) == 0);
}

// ==========================================================================
// Regression: init(nullptr, 0) after COMPLETE must not leave stale buffer
// ==========================================================================

TEST_CASE("PacketDeframer: init(nullptr,0) after COMPLETE unbinds buffer",
          "[m17][deframer]")
{
    // First packet: single-frame SMS that completes successfully.
    uint8_t rxBuf[M17_MAX_PACKET_DATA];
    PacketDeframer d;
    REQUIRE(d.init(rxBuf, sizeof(rxBuf)) == true);

    uint8_t appData[8] = { 0x05, 'H', 'i' };
    PacketFrame frames[4];
    size_t n = frameAll(appData, 3, frames, 4);
    REQUIRE(n == 1);
    REQUIRE(d.pushFrame(frames[0]) == DeframerResult::COMPLETE);

    // Simulate "no descriptor available": init with null/0.
    REQUIRE(d.init(nullptr, 0) == false);

    // A new incoming frame must NOT write into the old buffer.
    uint8_t appData2[8] = { 0x05, 'X' };
    PacketFrame frames2[4];
    size_t n2 = frameAll(appData2, 2, frames2, 4);
    REQUIRE(n2 == 1);
    REQUIRE(d.pushFrame(frames2[0]) == DeframerResult::ERR_OVERFLOW);

    // rxBuf must be unmodified from the first packet's data.
    REQUIRE(memcmp(rxBuf, appData, 3) == 0);
}
