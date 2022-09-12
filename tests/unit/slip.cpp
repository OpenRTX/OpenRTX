/*
 * SPDX-FileCopyrightText: Copyright 2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <catch2/catch_test_macros.hpp>
#include <array>
#include <cstring>
#include <errno.h>
#include "core/slip.h"

namespace
{

constexpr uint8_t END = 0xC0;
constexpr uint8_t ESC = 0xDB;
constexpr uint8_t ESC_END = 0xDC;
constexpr uint8_t ESC_ESC = 0xDD;

} // namespace

TEST_CASE("slip_init initializes object", "[slip]")
{
    std::array<uint8_t, 32> buffer{};
    struct slip sl{};

    REQUIRE(slip_init(&sl, buffer.data(), buffer.size()) == 0);

    CHECK(sl.buf == buffer.data());
    CHECK(sl.len == buffer.size());
    CHECK(sl.idx == 0);
}

TEST_CASE("slip_reset clears state", "[slip]")
{
    std::array<uint8_t, 32> buffer{};
    struct slip sl{};

    REQUIRE(slip_init(&sl, buffer.data(), buffer.size()) == 0);

    sl.idx = 12;
    sl.state = 99;

    slip_reset(&sl);

    CHECK(sl.idx == 0);
}

TEST_CASE("slip_alloc allocates object and buffer", "[slip]")
{
    struct slip *sl = slip_alloc(128);

    REQUIRE(sl != nullptr);

    CHECK(sl->buf != nullptr);
    CHECK(sl->len == 128);
    CHECK(sl->idx == 0);

    slip_free(sl);
}

TEST_CASE("encode simple frame", "[slip]")
{
    uint8_t payload[] = { 0x01, 0x02, 0x03 };

    struct slip sl{};
    REQUIRE(slip_init(&sl, payload, sizeof(payload)) == 0);

    uint8_t encoded[16] = {};

    auto ret = slip_encode(&sl, encoded, sizeof(encoded));

    REQUIRE(ret == 5);

    CHECK(encoded[0] == END);
    CHECK(encoded[1] == 0x01);
    CHECK(encoded[2] == 0x02);
    CHECK(encoded[3] == 0x03);
    CHECK(encoded[4] == END);
}

TEST_CASE("encode escapes END and ESC", "[slip]")
{
    uint8_t payload[] = { END, ESC };

    struct slip sl{};
    REQUIRE(slip_init(&sl, payload, sizeof(payload)) == 0);

    uint8_t encoded[16] = {};

    auto ret = slip_encode(&sl, encoded, sizeof(encoded));

    REQUIRE(ret == 6);

    CHECK(encoded[0] == END);

    CHECK(encoded[1] == ESC);
    CHECK(encoded[2] == ESC_END);

    CHECK(encoded[3] == ESC);
    CHECK(encoded[4] == ESC_ESC);

    CHECK(encoded[5] == END);
}

TEST_CASE("decode simple frame byte by byte", "[slip]")
{
    uint8_t decoded[16] = {};

    struct slip sl{};
    REQUIRE(slip_init(&sl, decoded, sizeof(decoded)) == 0);

    const uint8_t frame[] = { END, 0x11, 0x22, 0x33, END };

    int ret = 0;

    for (auto b : frame)
        ret = slip_decodeByte(&sl, b);

    REQUIRE(ret == 3);

    CHECK(sl.idx == 3);

    CHECK(decoded[0] == 0x11);
    CHECK(decoded[1] == 0x22);
    CHECK(decoded[2] == 0x33);
}

TEST_CASE("decode escaped characters", "[slip]")
{
    uint8_t decoded[16] = {};

    struct slip sl{};
    REQUIRE(slip_init(&sl, decoded, sizeof(decoded)) == 0);

    const uint8_t frame[] = { END, ESC, ESC_END, ESC, ESC_ESC, END };

    int ret = 0;

    for (auto b : frame)
        ret = slip_decodeByte(&sl, b);

    REQUIRE(ret == 2);

    CHECK(decoded[0] == END);
    CHECK(decoded[1] == ESC);
}

TEST_CASE("decode complete frame buffer", "[slip]")
{
    uint8_t decoded[16] = {};

    struct slip sl{};
    REQUIRE(slip_init(&sl, decoded, sizeof(decoded)) == 0);

    const uint8_t frame[] = { END, 0xAA, 0xBB, END };

    auto consumed = slip_decode(&sl, frame, sizeof(frame));

    CHECK(consumed == 4);
    CHECK(sl.idx == 2);

    CHECK(decoded[0] == 0xAA);
    CHECK(decoded[1] == 0xBB);
}

TEST_CASE("decoder reports buffer overflow", "[slip]")
{
    uint8_t decoded[2] = {};

    struct slip sl{};
    REQUIRE(slip_init(&sl, decoded, sizeof(decoded)) == 0);

    CHECK(slip_decodeByte(&sl, 0x01) == 0);
    CHECK(slip_decodeByte(&sl, 0x02) == 0);

    CHECK(slip_decodeByte(&sl, 0x03) == -ENOBUFS);
}

TEST_CASE("decoder rejects malformed escape sequence", "[slip]")
{
    uint8_t decoded[16] = {};

    struct slip sl{};
    REQUIRE(slip_init(&sl, decoded, sizeof(decoded)) == 0);

    REQUIRE(slip_decodeByte(&sl, ESC) == 0);

    CHECK(slip_decodeByte(&sl, 0x55) == -EPROTO);

    CHECK(slip_decodeByte(&sl, 0x01) == -EPERM);
}

TEST_CASE("encode decode roundtrip", "[slip]")
{
    const uint8_t original[] = { 0x01, END, 0x02, ESC, 0x03, 0x04 };

    uint8_t encodeStorage[sizeof(original)];
    struct slip encoder{};

    REQUIRE(slip_init(&encoder, encodeStorage, sizeof(encodeStorage)) == 0);

    std::memcpy(encodeStorage, original, sizeof(original));

    uint8_t encoded[64] = {};

    auto encodedLen = slip_encode(&encoder, encoded, sizeof(encoded));

    REQUIRE(encodedLen > 0);

    uint8_t decoded[32] = {};
    struct slip decoder{};

    REQUIRE(slip_init(&decoder, decoded, sizeof(decoded)) == 0);

    REQUIRE(slip_decode(&decoder, encoded, encodedLen) >= 0);

    REQUIRE(decoder.idx == sizeof(original));

    CHECK(std::memcmp(original, decoded, sizeof(original)) == 0);
}
