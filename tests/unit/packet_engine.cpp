/*
 * SPDX-FileCopyrightText: Copyright 2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <catch2/catch_test_macros.hpp>

#include <cerrno>
#include <cstring>

#include "core/packet_engine.h"
#include "interfaces/vcom.h"

static uint8_t written[64];
static size_t writtenLen;
static int writes;
static ssize_t writeResult;

/**
 * Virtual serial port stand-in: keeps what is written, and fails with
 * writeResult when it is negative.
 */
ssize_t vcom_writeBlock(const void *buf, size_t len)
{
    writes++;
    if (writeResult < 0)
        return writeResult;

    std::memcpy(written, buf, len);
    writtenLen = len;
    return (ssize_t)len;
}

/**
 * Start a test with nothing written yet.
 */
static void reset(void)
{
    writes = 0;
    writtenLen = 0;
    writeResult = 0;
}

TEST_CASE("packet engine: a frame goes out as one escaped KISS data frame",
          "[kiss]")
{
    const uint8_t frame[] = { 0x01, 0xC0, 0x02, 0xDB };
    const uint8_t expected[] = { 0xC0, 0x00, 0x01, 0xDB, 0xDC,
                                 0x02, 0xDB, 0xDD, 0xC0 };

    reset();
    REQUIRE(packetEngine_send(frame, sizeof(frame)) == 0);

    CHECK(writes == 1);
    REQUIRE(writtenLen == sizeof(expected));
    CHECK(std::memcmp(written, expected, sizeof(expected)) == 0);
}

TEST_CASE("packet engine: a failed write is reported", "[kiss]")
{
    const uint8_t frame[] = { 0x01, 0x02 };

    reset();
    writeResult = -ENOTCONN;
    CHECK(packetEngine_send(frame, sizeof(frame)) == -ENOTCONN);
}
