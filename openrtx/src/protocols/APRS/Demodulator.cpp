/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include "protocols/APRS/Demodulator.hpp"

using namespace APRS;

Demodulator::Demodulator() : newFrame(false)
{
}

Demodulator::~Demodulator()
{
}

void Demodulator::init()
{
    decoder.init();
    reset();
}

void Demodulator::terminate()
{
    decoder.terminate();
}

void Demodulator::reset()
{
    newFrame = false;
    decoder.reset();
    slicer.reset();
    bpf.reset();
    lpf.reset();
    markI.reset();
    markQ.reset();
    spaceI.reset();
    spaceQ.reset();
    dsp_resetState(dcBlock);
}

bool Demodulator::update(dataBlock_t baseband)
{
    // operate on the baseband audio sample-by-sample
    for (size_t i = 0; i < baseband.len; i++) {
        int16_t sample = dsp_dcBlockFilter(&dcBlock, baseband.data[i]);

        sample = demod(sample);

        int8_t bit = slicer.slice(sample);
        if (bit < 0)
            continue;

        newFrame = decoder.decode(bit);
    }

    return newFrame;
}

ssize_t Demodulator::getFrame(void *buf, size_t size)
{
    // Frame is consumed in any case
    newFrame = false;

    const auto &frame = decoder.getFrame();
    if (frame.len > size)
        return -EMSGSIZE;

    memcpy(buf, frame.data, frame.len);
    return static_cast<ssize_t>(frame.len);
}

bool Demodulator::getDCD() const
{
    return slicer.getDCD();
}

int16_t Demodulator::demod(int16_t input)
{
    // Input band-pass filter
    int32_t filtered = bpf(input);

    // Calculate mark and space correlations
    int32_t mark = std::abs(markI(filtered)) + std::abs(markQ(filtered));
    int32_t space = std::abs(spaceI(filtered)) + std::abs(spaceQ(filtered));

    // Output low-pass filter; the slicer only uses the sign
    int32_t out = lpf(mark - space);
    if (out > INT16_MAX)
        return INT16_MAX;
    if (out < INT16_MIN)
        return INT16_MIN;

    return static_cast<int16_t>(out);
}

constexpr std::array<int32_t, Demodulator::BPF_SIZE> Demodulator::BPF_TAPS;
constexpr std::array<int32_t, Demodulator::LPF_SIZE> Demodulator::LPF_TAPS;
constexpr std::array<int32_t, Demodulator::CORRELATOR_SIZE>
    Demodulator::MARK_CORRELATOR_I;
constexpr std::array<int32_t, Demodulator::CORRELATOR_SIZE>
    Demodulator::MARK_CORRELATOR_Q;
constexpr std::array<int32_t, Demodulator::CORRELATOR_SIZE>
    Demodulator::SPACE_CORRELATOR_I;
constexpr std::array<int32_t, Demodulator::CORRELATOR_SIZE>
    Demodulator::SPACE_CORRELATOR_Q;
