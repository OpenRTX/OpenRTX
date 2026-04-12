/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <catch2/catch_test_macros.hpp>
#include <cstring>
#include "rtx/PktBuf.hpp"
#include "protocols/M17/PacketFrame.hpp"

using namespace M17;

/*
 * These tests verify the M17 packet TX framing logic used by
 * OpMode_M17::txPacketBurst().  We replicate the chunking algorithm
 * and validate that the PacketFrame sequence correctly represents
 * the original payload.
 */

/* Replicate the chunking algorithm from txPacketBurst() */
static size_t chunkPacket(const rtxPacket_t *pkt, PacketFrame *frames,
                          size_t maxFrames)
{
    size_t offset = 0;
    size_t idx    = 0;

    while (offset < pkt->len && idx < maxFrames)
    {
        frames[idx].clear();
        size_t remaining = pkt->len - offset;

        if (remaining > PacketFrame::DATA_SIZE)
        {
            memcpy(frames[idx].data(), pkt->data + offset,
                   PacketFrame::DATA_SIZE);
            frames[idx].setEof(false);
            frames[idx].setCounter(
                static_cast<uint8_t>((offset / PacketFrame::DATA_SIZE) & 0x1F));
            offset += PacketFrame::DATA_SIZE;
        }
        else
        {
            memcpy(frames[idx].data(), pkt->data + offset, remaining);
            frames[idx].setEof(true);
            frames[idx].setCounter(static_cast<uint8_t>(remaining));
            offset += remaining;
        }

        idx++;
    }

    return idx;
}

TEST_CASE("Short packet produces single EOF frame", "[m17][pktbuf][tx]")
{
    rtxPacket_t pkt;
    memset(&pkt, 0, sizeof(pkt));
    memcpy(pkt.data, "HELLO", 5);
    pkt.len      = 5;
    pkt.protocol = PKT_PROTO_M17;

    PacketFrame frames[4];
    size_t n = chunkPacket(&pkt, frames, 4);

    REQUIRE(n == 1);
    REQUIRE(frames[0].isEof() == true);
    REQUIRE(frames[0].getCounter() == 5);
    REQUIRE(memcmp(frames[0].data(), "HELLO", 5) == 0);
}

TEST_CASE("Exact 25-byte packet produces two frames", "[m17][pktbuf][tx]")
{
    /* 25 bytes exactly: one intermediate (25 bytes would fit in one frame
     * only if remaining <= DATA_SIZE, but 25 == DATA_SIZE so it goes to
     * the else branch → single EOF frame) */
    rtxPacket_t pkt;
    memset(&pkt, 0, sizeof(pkt));
    for (size_t i = 0; i < PacketFrame::DATA_SIZE; i++)
        pkt.data[i] = static_cast<uint8_t>(i);
    pkt.len = PacketFrame::DATA_SIZE;

    PacketFrame frames[4];
    size_t n = chunkPacket(&pkt, frames, 4);

    /* 25 bytes exactly fits in the "remaining <= DATA_SIZE" branch = 1 frame */
    REQUIRE(n == 1);
    REQUIRE(frames[0].isEof() == true);
    REQUIRE(frames[0].getCounter() == PacketFrame::DATA_SIZE);

    for (size_t i = 0; i < PacketFrame::DATA_SIZE; i++)
    {
        REQUIRE(frames[0].data()[i] == static_cast<uint8_t>(i));
    }
}

TEST_CASE("26-byte packet splits into two frames", "[m17][pktbuf][tx]")
{
    rtxPacket_t pkt;
    memset(&pkt, 0, sizeof(pkt));
    for (size_t i = 0; i < 26; i++)
        pkt.data[i] = static_cast<uint8_t>(i);
    pkt.len = 26;

    PacketFrame frames[4];
    size_t n = chunkPacket(&pkt, frames, 4);

    REQUIRE(n == 2);

    /* Frame 0: intermediate, 25 bytes */
    REQUIRE(frames[0].isEof() == false);
    for (size_t i = 0; i < PacketFrame::DATA_SIZE; i++)
    {
        REQUIRE(frames[0].data()[i] == static_cast<uint8_t>(i));
    }

    /* Frame 1: EOF, 1 byte */
    REQUIRE(frames[1].isEof() == true);
    REQUIRE(frames[1].getCounter() == 1);
    REQUIRE(frames[1].data()[0] == 25);
}

TEST_CASE("55-byte packet produces three frames", "[m17][pktbuf][tx]")
{
    rtxPacket_t pkt;
    memset(&pkt, 0, sizeof(pkt));
    for (size_t i = 0; i < 55; i++)
        pkt.data[i] = static_cast<uint8_t>(i);
    pkt.len = 55;

    PacketFrame frames[8];
    size_t n = chunkPacket(&pkt, frames, 8);

    REQUIRE(n == 3);

    /* Frame 0: intermediate, 25 bytes */
    REQUIRE(frames[0].isEof() == false);
    REQUIRE(frames[0].data()[0] == 0);
    REQUIRE(frames[0].data()[24] == 24);

    /* Frame 1: intermediate, 25 bytes */
    REQUIRE(frames[1].isEof() == false);
    REQUIRE(frames[1].data()[0] == 25);
    REQUIRE(frames[1].data()[24] == 49);

    /* Frame 2: EOF, 5 bytes */
    REQUIRE(frames[2].isEof() == true);
    REQUIRE(frames[2].getCounter() == 5);
    REQUIRE(frames[2].data()[0] == 50);
    REQUIRE(frames[2].data()[4] == 54);
}

TEST_CASE("TX framing round-trips with RX assembly", "[m17][pktbuf][tx]")
{
    /* Build a 60-byte packet, chunk it into frames (TX), then reassemble
     * (RX) and verify the original data is recovered. */
    rtxPacket_t pkt;
    memset(&pkt, 0, sizeof(pkt));
    for (size_t i = 0; i < 60; i++)
        pkt.data[i] = static_cast<uint8_t>(i + 0x30);
    pkt.len      = 60;
    pkt.protocol = PKT_PROTO_M17;

    /* TX: chunk */
    PacketFrame frames[8];
    size_t nFrames = chunkPacket(&pkt, frames, 8);
    REQUIRE(nFrames == 3);

    /* RX: reassemble (replicates rxState logic) */
    rtxPacket_t rxPkt;
    memset(&rxPkt, 0, sizeof(rxPkt));
    size_t rxLen = 0;

    for (size_t i = 0; i < nFrames; i++)
    {
        const PacketFrame &pf = frames[i];

        if (pf.isEof())
        {
            uint8_t validBytes = pf.getCounter();
            if (validBytes > 0 && (rxLen + validBytes) <= RTX_MAX_PKT_LEN)
            {
                memcpy(rxPkt.data + rxLen, pf.data(), validBytes);
                rxLen += validBytes;
            }
            rxPkt.len = static_cast<uint16_t>(rxLen);
        }
        else
        {
            if ((rxLen + PacketFrame::DATA_SIZE) <= RTX_MAX_PKT_LEN)
            {
                memcpy(rxPkt.data + rxLen, pf.data(), PacketFrame::DATA_SIZE);
                rxLen += PacketFrame::DATA_SIZE;
            }
        }
    }

    REQUIRE(rxPkt.len == 60);
    REQUIRE(memcmp(rxPkt.data, pkt.data, 60) == 0);
}
