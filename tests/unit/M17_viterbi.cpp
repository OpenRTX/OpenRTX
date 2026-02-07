/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <random>
#include <array>
#include "protocols/M17/M17ConvolutionalEncoder.hpp"
#include "protocols/M17/M17CodePuncturing.hpp"
#include "protocols/M17/M17Viterbi.hpp"
#include "protocols/M17/M17Utils.hpp"

using namespace std;

static default_random_engine rng;

/**
 * Insert random bit flips in input data.
 */
template < size_t N >
static void generateErrors(array< uint8_t, N >& data)
{
    uniform_int_distribution< uint8_t > numErrs(0, 4);
    uniform_int_distribution< uint8_t > errPos(0, N);

    for(uint8_t i = 0; i < numErrs(rng); i++)
    {
        uint8_t pos = errPos(rng);
        bool    bit = M17::getBit(data, pos);
        M17::setBit(data, pos, !bit);
    }
}

TEST_CASE("Viterbi decode recovers punctured convolutional encoding", "[m17][viterbi]")
{
    uniform_int_distribution< uint8_t > rndValue(0, 255);

    array< uint8_t, 18 > source;

    for(auto& byte : source)
    {
        byte = rndValue(rng);
    }

    array<uint8_t, 37> encoded;
    M17::M17ConvolutionalEncoder encoder;
    encoder.reset();
    encoder.encode(source.data(), encoded.data(), source.size());
    encoded[36] = encoder.flush();

    array<uint8_t, 34> punctured;
    M17::puncture(encoded, punctured, M17::DATA_PUNCTURE);

    generateErrors(punctured);

    array< uint8_t, 18 > result;
    M17::M17HardViterbi decoder;
    decoder.decodePunctured(punctured, result, M17::DATA_PUNCTURE);

    for(size_t i = 0; i < result.size(); i++)
    {
        INFO("Position " << i << ": got 0x" << hex << static_cast<int>(result[i])
             << " expected 0x" << static_cast<int>(source[i]));
        REQUIRE(source[i] == result[i]);
    }
}
