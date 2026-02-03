/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef APRS_DEMODULATOR_H
#define APRS_DEMODULATOR_H

#ifndef __cplusplus
#error This header is C++ only!
#endif

#include <cstdint>
#include <cstddef>
#include "core/dsp.h"
#include "core/fir.hpp"
#include "core/audio_stream.h"
#include "protocols/APRS/constants.h"
#include "protocols/APRS/Decoder.hpp"
#include "protocols/APRS/Slicer.hpp"

namespace APRS
{

class Demodulator
{
public:
    /**
     * @brief Constructs a Demodulator object
     */
    Demodulator();

    /**
     * @brief Destroys a Demodulator object
     */
    ~Demodulator();

    /**
     * Allocate buffers and initialize demodulator.
     */
    void init();

    /**
     * Shutdown demodulator and deallocate data buffers.
     */
    void terminate();

    /**
     * Reset the demodulator state.
     */
    void reset();

    /**
     * @brief Updates demodulator state based on incoming baseband audio
     *
     * @param baseband: A dataBlock_t of baseband audio
     *
     * @return true if a new frame is ready, false otherwise
     */
    bool update(dataBlock_t baseband);

    /**
     * @brief Gets the ready frame and sets newFrame to false
     */
    struct frameData &getFrame();

private:
    /**
     * @brief Demodulates baseband audio by mark and space frequencies
     *
     * Calculates the difference between the magnitude of the mark tone and
     * the magnitued of the space tone using FIR filter when fed one sample at
     * a time
     *
     * @param input A int16_t baseband audio sample
     *
     * @return The difference between the mark and space tones
     */
    int16_t demod(const int16_t input);

    // Based on pymodem: https://github.com/ninocarrillo/pymodem
    // See https://github.com/rxt1077/aprs_prototype/ for how these were derived

    // input band-pass FIR filter
    static constexpr size_t BPF_SIZE = 7;
    static constexpr std::array<int16_t, BPF_SIZE> BPF_TAPS = { -1, -1, 1, 2,
                                                                1,  -1, -1 };
    Fir<int16_t, BPF_SIZE> bpf{ BPF_TAPS };

    // mark/space correlators
    static constexpr size_t CORRELATOR_SIZE = 8;
    static constexpr std::array<int16_t, CORRELATOR_SIZE> MARK_CORRELATOR_I = {
        8, 5, 0, -5, -8, -5, 0, 5
    };
    static constexpr std::array<int16_t, CORRELATOR_SIZE> MARK_CORRELATOR_Q = {
        0, 5, 8, 5, 0, -5, -8, -5
    };
    static constexpr std::array<int16_t, CORRELATOR_SIZE> SPACE_CORRELATOR_I = {
        8, 1, -7, -3, 6, 4, -5, -6
    };
    static constexpr std::array<int16_t, CORRELATOR_SIZE> SPACE_CORRELATOR_Q = {
        0, 7, 2, -7, -3, 6, 5, -4
    };
    Fir<int16_t, CORRELATOR_SIZE> markI{ MARK_CORRELATOR_I };
    Fir<int16_t, CORRELATOR_SIZE> markQ{ MARK_CORRELATOR_Q };
    Fir<int16_t, CORRELATOR_SIZE> spaceI{ SPACE_CORRELATOR_I };
    Fir<int16_t, CORRELATOR_SIZE> spaceQ{ SPACE_CORRELATOR_Q };

    // output low-pass FIR filter
    static constexpr size_t LPF_SIZE = 4;
    static constexpr std::array<int16_t, LPF_SIZE> LPF_TAPS = { 1, 2, 2, 1 };
    Fir<int16_t, LPF_SIZE> lpf{ LPF_TAPS };

    // state variables
    struct dcBlock dcBlock; ///< DC removal filter state
    int16_t lastSample = 0; ///< last sample used by demod
    int8_t clock = 0;       ///< slicer clock
    bool newFrame = false;  ///< flag for new frame available

    APRS::Slicer slicer;
    APRS::Decoder decoder;
};

} /* APRS */

#endif
