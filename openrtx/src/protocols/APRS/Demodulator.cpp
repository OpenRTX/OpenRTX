/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "protocols/APRS/Demodulator.hpp"
#include <cmath>

using namespace APRS;

Convolver::Convolver(const int8_t *impulseResponse, size_t impulseResponseSize)
{
    ir = impulseResponse;
    if (impulseResponseSize < 8) // maximum IR size is 8
        irSize = impulseResponseSize;
    else
        irSize = 8;
    overlapSize = irSize - 1;
}

Convolver::~Convolver()
{
}

#ifdef APRS_DEBUG
void Convolver::print()
{
    size_t i;

    printf("Convolver: irSize=%ld overlapSize=%ld ir=[", irSize, overlapSize);
    for (i = 0; i < irSize; i++)
        printf("%d,", ir[i]);
    printf("] output=[");
    for (i = 0; i < APRS_BUF_SIZE; i++)
        printf("%d,", output[i]);
    printf("] prevOverlap=[");
    for (i = 0; i < overlapSize; i++)
        printf("%d,", prevOverlap[i]);
    printf("] newOverlap=[");
    for (i = 0; i < overlapSize; i++)
        printf("%d,", newOverlap[i]);
    printf("]\n");
}
#endif

const int16_t *Convolver::convolve(const int16_t *input)
{
    size_t i, j;
    int16_t sample;

    // zero out the output and newOverlap buffers
    for (i = 0; i < APRS_BUF_SIZE; i++)
        output[i] = 0;
    for (i = 0; i < overlapSize; i++)
        newOverlap[i] = 0;

    // based on TABLE 6-1 'CONVOLUTION USING THE INPUT SIDE ALGORITHM'
    for (i = 0; i < (APRS_BUF_SIZE + irSize - 1); i++) {
        if (i < APRS_BUF_SIZE)
            sample = input[i];
        else
            // pad the end of the signal with zeroes
            sample = 0;
        for (j = 0; j < irSize; j++) {
            if ((i + j) < APRS_BUF_SIZE)
                output[i + j] += sample * ir[j];
            else {
                if ((i + j - APRS_BUF_SIZE) < overlapSize)
                    // the end of our convolution should go in the new_overlap buffer
                    newOverlap[i + j - APRS_BUF_SIZE] += sample * ir[j];
            }
        }
    }

    // add the previous overlap to the front of the output and swap
    // the overlap buffers
    for (i = 0; i < overlapSize; i++) {
        output[i] += prevOverlap[i];
        prevOverlap[i] = newOverlap[i];
    }

    return output;
}

// these are in protocols/APRS/Demodulator.hpp
constexpr size_t Demodulator::BPF_SIZE;
constexpr int8_t Demodulator::BPF_TAPS[];
constexpr size_t Demodulator::CORRELATOR_SIZE;
constexpr int8_t Demodulator::MARK_CORRELATOR_I[];
constexpr int8_t Demodulator::MARK_CORRELATOR_Q[];
constexpr int8_t Demodulator::SPACE_CORRELATOR_I[];
constexpr int8_t Demodulator::SPACE_CORRELATOR_Q[];
constexpr size_t Demodulator::LPF_SIZE;
constexpr int8_t Demodulator::LPF_TAPS[];

Demodulator::Demodulator()
{
}

Demodulator::~Demodulator()
{
}

const int16_t *Demodulator::demodulate(int16_t *input)
{
    // Input band-pass filter
    const int16_t *filteredAudio = bpf.convolve(input);

    // Create the correlation products
    const int16_t *markIResult = markI.convolve(filteredAudio);
    const int16_t *markQResult = markQ.convolve(filteredAudio);
    const int16_t *spaceIResult = spaceI.convolve(filteredAudio);
    const int16_t *spaceQResult = spaceQ.convolve(filteredAudio);

    // The demodulated signal is mark - space
    for (size_t i = 0; i < APRS_BUF_SIZE; i++) {
        int16_t mark = abs(markIResult[i]) + abs(markQResult[i]);
        int16_t space = abs(spaceIResult[i]) + abs(spaceQResult[i]);
        diff[i] = (mark - space) >> 3; // prevent overflow of int16_t
    }

    // Output low-pass filter
    return lpf.convolve(diff);
}
