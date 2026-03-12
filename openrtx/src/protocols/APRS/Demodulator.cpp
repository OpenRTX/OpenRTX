/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "protocols/APRS/Demodulator.hpp"
#include <cmath>

using namespace APRS;

// these are in protocols/APRS/Demodulator.hpp
constexpr std::array<int16_t, Demodulator::BPF_SIZE> Demodulator::BPF_TAPS;
constexpr std::array<int16_t, Demodulator::CORRELATOR_SIZE>
    Demodulator::MARK_CORRELATOR_I;
constexpr std::array<int16_t, Demodulator::CORRELATOR_SIZE>
    Demodulator::MARK_CORRELATOR_Q;
constexpr std::array<int16_t, Demodulator::CORRELATOR_SIZE>
    Demodulator::SPACE_CORRELATOR_I;
constexpr std::array<int16_t, Demodulator::CORRELATOR_SIZE>
    Demodulator::SPACE_CORRELATOR_Q;
constexpr std::array<int16_t, Demodulator::LPF_SIZE> Demodulator::LPF_TAPS;

Demodulator::Demodulator()
{
}

Demodulator::~Demodulator()
{
    terminate();
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
    lastSample = 0;
    clock = 0;
    decoder.reset();
    dsp_resetState(dcBlock);
}

int16_t Demodulator::demod(int16_t input)
{
    // Input band-pass filter
    int16_t filteredAudio = bpf(input);

    // Calculate mark and space correlations
    int16_t mark = abs(markI(filteredAudio)) + abs(markQ(filteredAudio));
    int16_t space = abs(spaceI(filteredAudio)) + abs(spaceQ(filteredAudio));
    int16_t diff = (mark - space) >> 3; // prevent overflow of int16_t

    // Output low-pass filter
    return lpf(diff);
}

struct frameData &Demodulator::getFrame()
{
    newFrame = false;
    return decoder.getFrame();
}

bool Demodulator::update(dataBlock_t baseband)
{
    // operate on the baseband audio sample-by-sample
    for (size_t i = 0; i < baseband.len; i++) {
        // apply the DC filter and scale the samples by 4 so our convolution
        // operations don't overflow. This seems to work well with the baseband
        // audio level
        int16_t sample = dsp_dcBlockFilter(&dcBlock, baseband.data[i]) >> 2;

        sample = demod(sample);

        int8_t bit = slicer.slice(sample);
        if (bit < 0)
            continue;

        newFrame = decoder.decode(bit);
    }
    return newFrame;
}
