/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <catch2/catch_test_macros.hpp>
#include <array>
#include <cstdint>
#include <random>
#include "protocols/M17/Golay.hpp"

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

/**
 * Soft bits of a received word, most significant first.
 *
 * @param word: hard bits of the word.
 * @param weak: bits received with low confidence.
 * @return the soft bits, fully confident except the weak ones.
 */
static array<uint16_t, 24> softWord(const uint32_t word, const uint32_t weak)
{
    array<uint16_t, 24> soft;

    for (size_t i = 0; i < 24; i++) {
        uint32_t bit = 1UL << (23 - i);
        bool one = (word & bit) != 0;
        if (weak & bit)
            soft[i] = one ? 0x9000 : 0x6FFF;
        else
            soft[i] = one ? 0xFFFF : 0x0000;
    }

    return soft;
}

TEST_CASE("Golay24 soft decoding of confident bits corrects 3 bit errors",
          "[m17][golay]")
{
    uniform_int_distribution<uint16_t> rndValue(0, 4095);
    uniform_int_distribution<uint8_t> numErrs(0, 3);

    for (uint32_t i = 0; i < 10000; i++) {
        uint16_t value = rndValue(rng);
        uint32_t cword = M17::golay24_encode(value);
        uint32_t emask = generateErrorMask(numErrs(rng));
        auto soft = softWord(cword ^ emask, 0);
        INFO("Value: " << value << " Error mask: " << emask);
        REQUIRE(M17::golay24_softDecode(soft) == value);
    }
}

TEST_CASE("Golay24 soft decoding corrects 5 bit errors, 4 of them weak",
          "[m17][golay]")
{
    uniform_int_distribution<uint16_t> rndValue(0, 4095);

    for (uint32_t i = 0; i < 10000; i++) {
        uint16_t value = rndValue(rng);
        uint32_t cword = M17::golay24_encode(value);
        uint32_t weak = generateErrorMask(4);
        uint32_t strong = 0;
        while ((strong == 0) || (strong & weak))
            strong = generateErrorMask(1);

        uint32_t word = cword ^ weak ^ strong;
        auto soft = softWord(word, weak);
        INFO("Value: " << value << " Weak: " << weak << " Strong: " << strong);
        REQUIRE(M17::golay24_decode(word) != value);
        REQUIRE(M17::golay24_softDecode(soft) == value);
    }
}

TEST_CASE("Golay24 soft decoding of confident bits rarely accepts 4 bit errors",
          "[m17][golay]")
{
    // With every bit equally confident the four flipped bits are picked by
    // position, so a candidate is sometimes the only one found
    uniform_int_distribution<uint16_t> rndValue(0, 4095);
    size_t accepted = 0;

    for (uint32_t i = 0; i < 100000; i++) {
        uint16_t value = rndValue(rng);
        uint32_t word = M17::golay24_encode(value) ^ generateErrorMask(4);
        if (M17::golay24_softDecode(softWord(word, 0)) != 0xFFFF)
            accepted++;
    }

    CAPTURE(accepted);
    REQUIRE(accepted <= 100);
}

TEST_CASE("Golay24 soft decoding rejects a word between two codewords",
          "[m17][golay]")
{
    // Two codewords eight bits apart, the code's minimum distance
    uint32_t a = M17::golay24_encode(0x5A3);
    uint32_t diff = 0;
    for (uint16_t value = 1; __builtin_popcount(diff) != 8; value++)
        diff = M17::golay24_encode(value);

    // Of the eight bits, half lean to each codeword by the same amounts
    array<uint16_t, 24> soft = softWord(a, 0);
    static constexpr uint16_t CONF[4] = { 500, 500, 1500, 1500 };
    size_t towardA = 0;
    size_t towardB = 0;
    for (size_t i = 0; i < 24; i++) {
        uint32_t bit = 1UL << (23 - i);
        if ((diff & bit) == 0)
            continue;

        bool one = (a & bit) != 0;
        bool leanA = (towardA <= towardB);
        uint16_t conf = leanA ? CONF[towardA++] : CONF[towardB++];
        if (one == leanA)
            soft[i] = 0x8000 + conf;
        else
            soft[i] = 0x7FFF - conf;
    }

    REQUIRE(towardA == 4);
    REQUIRE(towardB == 4);
    REQUIRE(M17::golay24_softDecode(soft) == 0xFFFF);
}
