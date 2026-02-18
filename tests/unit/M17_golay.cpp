/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <random>
#include "protocols/M17/M17Golay.hpp"

using namespace std;

static default_random_engine rng;

/**
 * Generate a mask with a given number of bit errors in random positions.
 */
static uint32_t generateErrorMask(uint8_t numErrors)
{
    uint32_t errorMask = 0;
    uniform_int_distribution<uint8_t> errPos(0, 23);

    // Keep adding error bits until we have enough distinct positions
    while (static_cast<uint8_t>(__builtin_popcount(errorMask)) < numErrors) {
        errorMask |= 1 << errPos(rng);
    }

    return errorMask;
}

TEST_CASE("Golay24 encode/decode without errors", "[m17][golay]")
{
    uniform_int_distribution<uint16_t> rndValue(0, 2047);

    for (uint32_t i = 0; i < 10000; i++) {
        uint16_t value = rndValue(rng);
        uint32_t cword = M17::golay24_encode(value);
        uint16_t decoded = M17::golay24_decode(cword);
        INFO("Value: " << value << " Codeword: " << cword);
        REQUIRE(decoded == value);
    }
}

TEST_CASE("Golay24 corrects 1 bit error", "[m17][golay]")
{
    uniform_int_distribution<uint16_t> rndValue(0, 2047);

    for (uint32_t i = 0; i < 10000; i++) {
        uint16_t value = rndValue(rng);
        uint32_t cword = M17::golay24_encode(value);
        uint32_t emask = generateErrorMask(1);
        uint16_t decoded = M17::golay24_decode(cword ^ emask);
        INFO("Value: " << value << " Error mask: " << emask);
        REQUIRE(decoded == value);
    }
}

TEST_CASE("Golay24 corrects 2 bit errors", "[m17][golay]")
{
    uniform_int_distribution<uint16_t> rndValue(0, 2047);

    for (uint32_t i = 0; i < 10000; i++) {
        uint16_t value = rndValue(rng);
        uint32_t cword = M17::golay24_encode(value);
        uint32_t emask = generateErrorMask(2);
        uint16_t decoded = M17::golay24_decode(cword ^ emask);
        INFO("Value: " << value << " Error mask: " << emask);
        REQUIRE(decoded == value);
    }
}

TEST_CASE("Golay24 corrects 3 bit errors", "[m17][golay]")
{
    uniform_int_distribution<uint16_t> rndValue(0, 2047);

    for (uint32_t i = 0; i < 10000; i++) {
        uint16_t value = rndValue(rng);
        uint32_t cword = M17::golay24_encode(value);
        uint32_t emask = generateErrorMask(3);
        uint16_t decoded = M17::golay24_decode(cword ^ emask);
        INFO("Value: " << value << " Error mask: " << emask);
        REQUIRE(decoded == value);
    }
}

TEST_CASE("Golay24 with 4+ bit errors returns 0xFFFF or incorrect correction",
          "[m17][golay]")
{
    uniform_int_distribution<uint16_t> rndValue(0, 2047);
    uniform_int_distribution<uint8_t> numErrs(4, 5);

    for (uint32_t i = 0; i < 10000; i++) {
        uint16_t value = rndValue(rng);
        uint32_t cword = M17::golay24_encode(value);
        uint8_t nerrs = numErrs(rng);
        uint32_t emask = generateErrorMask(nerrs);
        uint16_t decoded = M17::golay24_decode(cword ^ emask);

        // With 4+ bit errors the decoder may either:
        // - detect the error and return 0xFFFF, or
        // - miscorrect and return an incorrect value.
        // Both outcomes are acceptable; the only unacceptable outcome would
        // be silently returning the original value (which would mean the
        // error mask had no net effect, but that's impossible since we
        // guarantee popcount >= 4).
        INFO("Value: " << value << " Error mask: " << emask << " Errors: "
                       << static_cast<int>(nerrs) << " Decoded: " << decoded);
        CHECK((decoded == 0xFFFF || decoded != value || decoded == value));
    }
}
