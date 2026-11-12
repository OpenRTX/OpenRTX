/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <array>
#include <algorithm>
#include <random>

#include "protocols/M17/FrameEncoder.hpp"
#include "protocols/M17/FrameDecoder.hpp"
#include "protocols/M17/Constants.hpp"
#include "protocols/M17/Datatypes.hpp"
#include "protocols/M17/LinkSetupFrame.hpp"
#include "protocols/M17/Utils.hpp"

using namespace M17;

/**
 * Soft bits of a frame, every bit fully confident.
 *
 * @param frame: frame.
 * @return the soft bits.
 */
static softFrame_t softBits(const frame_t &frame)
{
    softFrame_t soft;
    for (size_t i = 0; i < soft.size(); i++)
        soft[i] = getBit(frame, i) ? UINT16_MAX : 0;

    return soft;
}

/**
 * Standard normal sample by Box-Muller on the raw mt19937 output, which the
 * standard fully specifies, unlike std::normal_distribution.
 *
 * @param rng: generator.
 * @return the sample.
 */
static double gaussian(std::mt19937 &rng)
{
    constexpr double twoPi = 6.283185307179586;
    double u1 = (static_cast<double>(rng()) + 1.0) / 4294967297.0;
    double u2 = static_cast<double>(rng()) / 4294967296.0;
    return std::sqrt(-2.0 * std::log(u1)) * std::cos(twoPi * u2);
}

/**
 * Reset a decoder and prime it with the LSF the test stream frames belong to.
 *
 * @param dec: decoder.
 */
static void feedTestLsf(FrameDecoder &dec)
{
    dec.reset();

    FrameEncoder enc;
    LinkSetupFrame lsf;
    lsf.clear();
    lsf.setSource("N0CALL");
    lsf.setDestination("ALL");
    frame_t lsfFrame;
    enc.encodeLsf(lsf, lsfFrame);
    dec.decodeFrame(softBits(lsfFrame));
}

/**
 * Slice a sample at ideal symbol levels into the two soft bits of symbol i,
 * mapped the way the demodulator maps them.
 *
 * @param x: sample, with the +1 level at unit.
 * @param unit: level of the +1 symbol.
 * @param i: symbol index in the frame.
 * @param soft: soft bits.
 */
static void sliceSymbol(const int16_t x, const int32_t unit, const size_t i,
                        softFrame_t &soft)
{
    // Outer symbols lie beyond two units; each bit is fully confident one
    // unit from its boundary (distances tripled, as in the demodulator)
    int8_t symbol;
    if (x > 2 * unit)
        symbol = +3;
    else if (x < -2 * unit)
        symbol = -3;
    else if (x > 0)
        symbol = +1;
    else
        symbol = -1;

    bool outerSym = (symbol == +3) || (symbol == -3);
    int32_t outerThr = (symbol < 0) ? -2 * unit : 2 * unit;
    uint32_t signDist = 3 * std::abs(static_cast<int32_t>(x));
    uint32_t outerDist = 3 * std::abs(x - outerThr);

    soft[2 * i] = softBit(symbol < 0, signDist, 3 * unit);
    soft[2 * i + 1] = softBit(outerSym, outerDist, 3 * unit);
}

/**
 * Demodulate a frame through Gaussian noise on its symbols, sliced at ideal
 * symbol levels. The syncword stays clean.
 *
 * @param rng: noise generator.
 * @param sigma: noise rms, in units of the +1 symbol level.
 * @param frame: encoded frame.
 * @param soft: soft bits.
 */
static void demodulate(std::mt19937 &rng, const double sigma,
                       const frame_t &frame, softFrame_t &soft)
{
    static constexpr int32_t UNIT = 1000;

    for (size_t i = 0; i < FRAME_SYMBOLS; i++) {
        float level = byteToSymbols(frame[i / 4])[i % 4];
        if ((i >= SYNCWORD_SYMBOLS) && (sigma > 0.0))
            level += static_cast<float>(sigma * gaussian(rng));
        float x = std::max(-32768.0f, std::min(32767.0f, level * UNIT));

        sliceSymbol(static_cast<int16_t>(x), UNIT, i, soft);
    }
}

/**
 * Build a random stream frame and demodulate it through noise.
 *
 * @param rng: payload and noise generator.
 * @param sigma: noise rms, in units of the +1 symbol level.
 * @param dec: decoder, primed with the LSF the frame belongs to.
 * @param payload: payload of the frame.
 * @param soft: soft bits.
 */
static void makeNoisyStreamFrame(std::mt19937 &rng, double sigma,
                                 FrameDecoder &dec, payload_t &payload,
                                 softFrame_t &soft)
{
    feedTestLsf(dec);

    for (auto &b : payload)
        b = rng() & 0xFF;
    FrameEncoder enc;
    frame_t frame;
    enc.encodeStreamFrame(payload, frame, false);
    demodulate(rng, sigma, frame, soft);
}

TEST_CASE("Soft decoder accepts a noisy but decodable stream frame",
          "[m17][decoder][soft]")
{
    std::mt19937 rng(42);
    for (int t = 0; t < 50; t++) {
        FrameDecoder dec;
        payload_t payload;
        softFrame_t soft;
        makeNoisyStreamFrame(rng, 0.5, dec, payload, soft);
        REQUIRE(dec.decodeFrame(soft) == FrameType::STREAM);
        REQUIRE(
            memcmp(dec.getStreamFrame().data(), payload.data(), payload.size())
            == 0);
    }
}

TEST_CASE("Soft decoder rejects a stream payload decoded from noise",
          "[m17][decoder][soft]")
{
    std::mt19937 rng(7);
    for (int t = 0; t < 50; t++) {
        FrameDecoder dec;
        payload_t payload;
        softFrame_t soft;
        makeNoisyStreamFrame(rng, 0.0, dec, payload, soft);
        // Noise behind a valid stream sync word
        for (size_t i = 2 * SYNCWORD_SYMBOLS; i < soft.size(); i++)
            soft[i] = static_cast<uint16_t>(rng());
        payload_t before;
        memcpy(before.data(), dec.getStreamFrame().data(), before.size());
        REQUIRE(dec.decodeFrame(soft) == FrameType::STREAM);
        REQUIRE(
            memcmp(dec.getStreamFrame().data(), before.data(), before.size())
            == 0);
    }
}

TEST_CASE("Clean symbols slice to fully confident soft bits and decode",
          "[m17][decoder][soft]")
{
    std::mt19937 rng(99);
    for (int t = 0; t < 50; t++) {
        payload_t payload;
        for (auto &b : payload)
            b = rng() & 0xFF;
        FrameEncoder enc;
        frame_t frame;
        enc.encodeStreamFrame(payload, frame, false);

        softFrame_t soft;
        demodulate(rng, 0.0, frame, soft);
        REQUIRE(soft == softBits(frame));

        FrameDecoder dec;
        feedTestLsf(dec);
        REQUIRE(dec.decodeFrame(soft) == FrameType::STREAM);
        REQUIRE(
            memcmp(dec.getStreamFrame().data(), payload.data(), payload.size())
            == 0);
    }
}

TEST_CASE("Soft decoder recovers most link setup frames through noise",
          "[m17][decoder][soft]")
{
    // At this noise level about 66% decode with the sign bit of an inner
    // symbol fully confident, 50% with a third of that confidence.
    static constexpr size_t TRIALS = 400;
    std::mt19937 rng(1000);
    FrameDecoder dec;
    size_t decoded = 0;

    for (size_t t = 0; t < TRIALS; t++) {
        LinkSetupFrame lsf;
        lsf.clear();
        lsf.setSource("N0CALL");
        lsf.setDestination("ALL");
        for (auto &b : lsf.metadata().raw_data)
            b = rng() & 0xFF;

        FrameEncoder enc;
        frame_t frame;
        softFrame_t soft;
        enc.encodeLsf(lsf, frame);
        demodulate(rng, 0.7, frame, soft);

        REQUIRE(dec.decodeFrame(soft) == FrameType::LINK_SETUP);
        LinkSetupFrame got = dec.getLsf();
        if (got.valid()
            && (memcmp(got.getData(), lsf.getData(), sizeof(lsf)) == 0))
            decoded++;
    }

    CAPTURE(decoded);
    REQUIRE(decoded >= TRIALS * 53 / 100);
}

TEST_CASE("Soft decoder accepts most stream frames through noise",
          "[m17][decoder][soft]")
{
    // At this noise level about 87% decode correctly and pass the limit, but
    // only 64% would pass a limit of 17.
    static constexpr size_t TRIALS = 400;
    std::mt19937 rng(2000);
    size_t accepted = 0;

    for (size_t t = 0; t < TRIALS; t++) {
        FrameDecoder dec;
        payload_t payload;
        softFrame_t soft;
        makeNoisyStreamFrame(rng, 0.7, dec, payload, soft);
        dec.decodeFrame(soft);
        if (memcmp(dec.getStreamFrame().data(), payload.data(), payload.size())
            == 0)
            accepted++;
    }

    CAPTURE(accepted);
    REQUIRE(accepted >= TRIALS * 75 / 100);
}

TEST_CASE("Soft decoder assembles the LSF from the LICH through noise",
          "[m17][decoder][soft]")
{
    // A receiver that missed the LSF frame needs all six LICH chunks. At this
    // noise level one cycle of six stream frames assembles about 85% of LSFs.
    static constexpr size_t TRIALS = 200;
    std::mt19937 rng(3000);
    size_t assembled = 0;

    for (size_t t = 0; t < TRIALS; t++) {
        LinkSetupFrame lsf;
        lsf.clear();
        lsf.setSource("N0CALL");
        lsf.setDestination("ALL");
        for (auto &b : lsf.metadata().raw_data)
            b = rng() & 0xFF;

        FrameEncoder enc;
        frame_t lsfFrame;
        enc.encodeLsf(lsf, lsfFrame);

        FrameDecoder dec;
        dec.reset();
        for (size_t f = 0; f < 6; f++) {
            payload_t payload;
            for (auto &b : payload)
                b = rng() & 0xFF;

            frame_t frame;
            softFrame_t soft;
            enc.encodeStreamFrame(payload, frame, false);
            demodulate(rng, 0.7, frame, soft);
            dec.decodeFrame(soft);
        }

        LinkSetupFrame got = dec.getLsf();
        if (got.valid()
            && (memcmp(got.getData(), lsf.getData(), sizeof(lsf)) == 0))
            assembled++;
    }

    CAPTURE(assembled);
    REQUIRE(assembled >= TRIALS * 60 / 100);
}
