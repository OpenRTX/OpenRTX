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
 * These tests verify the M17 packet assembly logic that lives in
 * OpMode_M17::rxState().  Because the OpMode is tightly coupled to
 * hardware (radio, demodulator), we replicate the assembly algorithm
 * from rxState() in a small helper and drive it with hand-crafted
 * PacketFrame sequences.
 */

static void assemblePacket(const PacketFrame frames[], size_t nFrames,
                           PktBuf *q)
{
    rtxPacket_t rxPacket;
    memset(&rxPacket, 0, sizeof(rxPacket));
    size_t rxPacketLen = 0;

    for (size_t i = 0; i < nFrames; i++)
    {
        const PacketFrame &pf = frames[i];

        if (pf.isEof())
        {
            uint8_t validBytes = pf.getCounter();
            if (validBytes > 0
                && (rxPacketLen + validBytes) <= RTX_MAX_PKT_LEN)
            {
                memcpy(rxPacket.data + rxPacketLen, pf.data(), validBytes);
                rxPacketLen += validBytes;
            }

            rxPacket.len      = static_cast<uint16_t>(rxPacketLen);
            rxPacket.protocol = PKT_PROTO_M17;
            rxPacket.rssi     = -80;
            rxPacket.timestamp = 0;

            if (rxPacketLen > 0)
                q->push(&rxPacket);

            rxPacketLen = 0;
            memset(&rxPacket, 0, sizeof(rxPacket));
        }
        else
        {
            if ((rxPacketLen + PacketFrame::DATA_SIZE) <= RTX_MAX_PKT_LEN)
            {
                memcpy(rxPacket.data + rxPacketLen, pf.data(),
                       PacketFrame::DATA_SIZE);
                rxPacketLen += PacketFrame::DATA_SIZE;
            }
        }
    }
}

TEST_CASE("Single-frame M17 packet", "[m17][pktbuf]")
{
    PktBuf q;

    /* Build one EOF frame with 10 valid bytes */
    PacketFrame frame;
    frame.clear();
    for (size_t i = 0; i < 10; i++)
        frame[i] = static_cast<uint8_t>(0xA0 + i);
    frame.setEof(true);
    frame.setCounter(10);

    assemblePacket(&frame, 1, &q);

    REQUIRE(q.pending() == 1);

    rtxPacket_t out;
    REQUIRE(q.pop(&out) == true);
    REQUIRE(out.len == 10);
    REQUIRE(out.protocol == PKT_PROTO_M17);
    REQUIRE(out.data[0] == 0xA0);
    REQUIRE(out.data[9] == 0xA9);
}

TEST_CASE("Multi-frame M17 packet", "[m17][pktbuf]")
{
    PktBuf q;

    /* Two intermediate frames (25 bytes each) + one EOF with 5 valid bytes
     * = 55 bytes total */
    PacketFrame frames[3];

    /* Frame 0: intermediate, full 25 bytes */
    frames[0].clear();
    for (size_t i = 0; i < PacketFrame::DATA_SIZE; i++)
        frames[0][i] = static_cast<uint8_t>(i);
    frames[0].setEof(false);
    frames[0].setCounter(0);

    /* Frame 1: intermediate, full 25 bytes */
    frames[1].clear();
    for (size_t i = 0; i < PacketFrame::DATA_SIZE; i++)
        frames[1][i] = static_cast<uint8_t>(i + 25);
    frames[1].setEof(false);
    frames[1].setCounter(1);

    /* Frame 2: EOF with 5 valid bytes */
    frames[2].clear();
    for (size_t i = 0; i < 5; i++)
        frames[2][i] = static_cast<uint8_t>(i + 50);
    frames[2].setEof(true);
    frames[2].setCounter(5);

    assemblePacket(frames, 3, &q);

    REQUIRE(q.pending() == 1);

    rtxPacket_t out;
    REQUIRE(q.pop(&out) == true);
    REQUIRE(out.len == 55);

    /* Verify byte continuity */
    for (uint16_t i = 0; i < 55; i++)
    {
        REQUIRE(out.data[i] == static_cast<uint8_t>(i));
    }
}

TEST_CASE("Two consecutive M17 packets", "[m17][pktbuf]")
{
    PktBuf q;

    PacketFrame frames[2];

    /* Packet 1: single EOF frame, 3 bytes */
    frames[0].clear();
    frames[0][0] = 0x11;
    frames[0][1] = 0x22;
    frames[0][2] = 0x33;
    frames[0].setEof(true);
    frames[0].setCounter(3);

    /* Packet 2: single EOF frame, 2 bytes */
    frames[1].clear();
    frames[1][0] = 0xAA;
    frames[1][1] = 0xBB;
    frames[1].setEof(true);
    frames[1].setCounter(2);

    assemblePacket(frames, 2, &q);

    REQUIRE(q.pending() == 2);

    rtxPacket_t out;
    REQUIRE(q.pop(&out) == true);
    REQUIRE(out.len == 3);
    REQUIRE(out.data[0] == 0x11);

    REQUIRE(q.pop(&out) == true);
    REQUIRE(out.len == 2);
    REQUIRE(out.data[0] == 0xAA);
}
