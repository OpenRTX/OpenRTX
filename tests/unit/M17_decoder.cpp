/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <array>
#include <algorithm>
#include <random>
#include "protocols/M17/FrameEncoder.hpp"
#include "protocols/M17/FrameDecoder.hpp"
#include "protocols/M17/Datatypes.hpp"
#include "protocols/M17/LinkSetupFrame.hpp"

using namespace M17;

// Standard normal sample by Box-Muller on the raw mt19937 output. The engine
// is fully specified by the standard while std::normal_distribution is not, so
// this keeps the test frames identical across C++ libraries.
static double gaussian(std::mt19937 &rng)
{
    constexpr double twoPi = 6.283185307179586;
    double u1 = (static_cast<double>(rng()) + 1.0) / 4294967297.0;
    double u2 = static_cast<double>(rng()) / 4294967296.0;
    return std::sqrt(-2.0 * std::log(u1)) * std::cos(twoPi * u2);
}

// Prime a decoder with the LSF the test stream frames belong to.
static void feedTestLsf(FrameDecoder &dec)
{
    FrameEncoder enc;
    LinkSetupFrame lsf;
    lsf.clear();
    lsf.setSource("N0CALL");
    lsf.setDestination("ALL");
    frame_t lsfFrame;
    enc.encodeLsf(lsf, lsfFrame);
    dec.decodeFrame(lsfFrame);
}

// Build one encoded stream frame and, from it, the hard frame and soft bits a
// demodulator would produce for a given per-symbol Gaussian noise level (in
// symbol units, ideal levels +-1/+-3). The soft mapping mirrors
// Demodulator::quantize(): sign -> first bit, magnitude -> second bit.
static void makeNoisyStreamFrame(std::mt19937 &rng, double sigma,
                                 FrameDecoder &dec, payload_t &payload,
                                 frame_t &hard, softFrame_t &soft)
{
    feedTestLsf(dec);

    for (auto &b : payload)
        b = rng() & 0xFF;
    FrameEncoder enc;
    frame_t frame;
    enc.encodeStreamFrame(payload, frame, false);
    hard = frame;

    for (size_t i = 0; i < 192; i++) {
        uint8_t d = (frame[i / 4] >> (6 - 2 * (i % 4))) & 3;
        float level = (d == 1) ? 3 : (d == 0) ? 1 : (d == 2) ? -1 : -3;
        // Keep the sync word clean, add noise to the payload
        if ((i >= 8) && (sigma > 0.0))
            level += static_cast<float>(sigma * gaussian(rng));
        float lv = std::max(-3.0f, std::min(3.0f, level));
        float sign = 0.5f - lv / 6.0f;
        float mag = std::max(0.0f,
                             std::min(1.0f, (std::fabs(lv) - 1.0f) / 2.0f));
        soft[2 * i] = static_cast<uint16_t>(sign * 65535.0f);
        soft[2 * i + 1] = static_cast<uint16_t>(mag * 65535.0f);
        uint8_t hd = level > 2 ? 1 : level > 0 ? 0 : level > -2 ? 2 : 3;
        hard[i / 4] = (hard[i / 4] & ~(3 << (6 - 2 * (i % 4))))
                    | (hd << (6 - 2 * (i % 4)));
    }
}

TEST_CASE("Soft decoder accepts a noisy but decodable stream frame",
          "[m17][decoder][soft]")
{
    std::mt19937 rng(42);
    for (int t = 0; t < 50; t++) {
        FrameDecoder dec;
        dec.init();
        payload_t payload;
        frame_t hard;
        softFrame_t soft;
        makeNoisyStreamFrame(rng, 0.5, dec, payload, hard, soft);
        REQUIRE(dec.decodeFrame(hard, soft) == FrameType::STREAM);
        REQUIRE(dec.getStreamBitErrors() < FrameDecoder::MAX_STREAM_BIT_ERRORS);
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
        dec.init();
        payload_t payload;
        frame_t hard;
        softFrame_t soft;
        makeNoisyStreamFrame(rng, 0.0, dec, payload, hard, soft);
        // Replace everything after the sync word with random soft bits and
        // the matching hard decisions: noise behind a valid stream sync word.
        for (size_t i = 16; i < soft.size(); i++) {
            soft[i] = static_cast<uint16_t>(rng() & 0xFFFF);
            bool bit = soft[i] >= 0x8000;
            size_t byte = i / 8, sh = 7 - (i % 8);
            hard[byte] = (hard[byte] & ~(1 << sh)) | (bit << sh);
        }
        payload_t before;
        memcpy(before.data(), dec.getStreamFrame().data(), before.size());
        REQUIRE(dec.decodeFrame(hard, soft) == FrameType::STREAM);
        REQUIRE(dec.getStreamBitErrors()
                >= FrameDecoder::MAX_STREAM_BIT_ERRORS);
        REQUIRE(
            memcmp(dec.getStreamFrame().data(), before.data(), before.size())
            == 0);
    }
}

TEST_CASE("Soft decoder on a clean frame matches the hard-input path",
          "[m17][decoder][soft]")
{
    std::mt19937 rng(99);
    for (int t = 0; t < 50; t++) {
        FrameDecoder decSoft, decHard;
        decSoft.init();
        decHard.init();
        payload_t payload;
        frame_t hard;
        softFrame_t soft;
        makeNoisyStreamFrame(rng, 0.0, decSoft, payload, hard, soft);
        feedTestLsf(decHard);
        REQUIRE(decSoft.decodeFrame(hard, soft) == FrameType::STREAM);
        REQUIRE(decHard.decodeFrame(hard) == FrameType::STREAM);
        REQUIRE(decSoft.getStreamBitErrors() == 0);
        REQUIRE(decHard.getStreamBitErrors() == 0);
        REQUIRE(memcmp(decSoft.getStreamFrame().data(),
                       decHard.getStreamFrame().data(), payload.size())
                == 0);
    }
}
