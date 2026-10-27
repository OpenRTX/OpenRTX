/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <cmath>
#include <random>
#include <array>
#include "protocols/M17/ConvolutionalEncoder.hpp"
#include "protocols/M17/CodePuncturing.hpp"
#include "protocols/M17/Viterbi.hpp"
#include "protocols/M17/Utils.hpp"

using namespace std;

static default_random_engine rng;

/**
 * Standard normal sample by Box-Muller on the raw mt19937 output, which the
 * standard fully specifies, unlike std::normal_distribution.
 *
 * @param gen: generator.
 * @return the sample.
 */
static double gaussian(mt19937 &gen)
{
    constexpr double twoPi = 6.283185307179586;
    double u1 = (static_cast<double>(gen()) + 1.0) / 4294967297.0;
    double u2 = static_cast<double>(gen()) / 4294967296.0;
    return sqrt(-2.0 * log(u1)) * cos(twoPi * u2);
}

/**
 * Insert random bit flips in input data.
 */
template <size_t N>
static void generateErrors(array<uint8_t, N> &data)
{
    uniform_int_distribution<uint8_t> numErrs(0, 4);
    uniform_int_distribution<uint8_t> errPos(0, N);

    for (uint8_t i = 0; i < numErrs(rng); i++) {
        uint8_t pos = errPos(rng);
        bool bit = M17::getBit(data, pos);
        M17::setBit(data, pos, !bit);
    }
}

TEST_CASE("Viterbi decode recovers punctured convolutional encoding",
          "[m17][viterbi]")
{
    uniform_int_distribution<uint8_t> rndValue(0, 255);

    array<uint8_t, 18> source;

    for (auto &byte : source) {
        byte = rndValue(rng);
    }

    array<uint8_t, 37> encoded;
    M17::ConvolutionalEncoder encoder;
    encoder.reset();
    encoder.encode(source.data(), encoded.data(), source.size());
    encoded[36] = encoder.flush();

    array<uint8_t, 34> punctured;
    M17::puncture(encoded, punctured, M17::DATA_PUNCTURE);

    generateErrors(punctured);

    // Hard bits as fully confident soft values
    array<uint16_t, 34 * 8> soft;
    for (size_t i = 0; i < soft.size(); i++)
        soft[i] = M17::getBit(punctured, i) ? 0xFFFF : 0x0000;

    array<uint8_t, 18> result;
    M17::SoftViterbi decoder;
    decoder.decodePunctured(soft, result, M17::DATA_PUNCTURE);

    for (size_t i = 0; i < result.size(); i++) {
        INFO("Position " << i << ": got 0x" << hex
                         << static_cast<int>(result[i]) << " expected 0x"
                         << static_cast<int>(source[i]));
        REQUIRE(source[i] == result[i]);
    }
}

TEST_CASE("Viterbi decode starts the trellis in the encoder's start state",
          "[m17][viterbi]")
{
    // LSF-sized blocks through Gaussian noise on the soft bits. Knowing that
    // the encoder starts in state zero decodes 65% of them; letting every
    // state start equal decodes 58%.
    static constexpr size_t TRIALS = 400;
    mt19937 gen(3000);
    size_t decoded = 0;

    for (size_t t = 0; t < TRIALS; t++) {
        array<uint8_t, 30> source;
        for (auto &byte : source)
            byte = gen() & 0xFF;

        array<uint8_t, 61> encoded;
        M17::ConvolutionalEncoder encoder;
        encoder.reset();
        encoder.encode(source.data(), encoded.data(), source.size());
        encoded[60] = encoder.flush();

        array<uint8_t, 46> punctured;
        M17::puncture(encoded, punctured, M17::LSF_PUNCTURE);

        array<uint16_t, 46 * 8> soft;
        for (size_t i = 0; i < soft.size(); i++) {
            double v = (M17::getBit(punctured, i) ? 1.0 : 0.0)
                     + 0.32 * gaussian(gen);
            soft[i] = static_cast<uint16_t>(65535.0 * min(1.0, max(0.0, v)));
        }

        array<uint8_t, 30> result;
        M17::SoftViterbi decoder;
        decoder.decodePunctured(soft, result, M17::LSF_PUNCTURE);
        if (result == source)
            decoded++;
    }

    CAPTURE(decoded);
    REQUIRE(decoded >= 250);
}
