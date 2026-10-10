/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef M17SIGNAL_H
#define M17SIGNAL_H

#ifndef __cplusplus
#error This header is C++ only!
#endif

#include <cstddef>
#include <cstdint>
#include <vector>

#include "protocols/M17/Constants.hpp"
#include "protocols/M17/Datatypes.hpp"
#include "Channel.hpp"

/**
 * M17 test signals: frames encoded by FrameEncoder, their symbols, and the
 * baseband reaching the demodulator.
 */
namespace M17Signal
{

/** RRC-shaped 24 kHz baseband fed straight to the demodulator. */
std::vector<int16_t> rrcBaseband(const std::vector<int8_t> &symbols,
                                 const float amplitude = 2000.0f);

/** Append preamble symbols, alternating +3 and -3. */
void appendPreamble(std::vector<int8_t> &symbols,
                    const size_t count = M17::FRAME_SYMBOLS);

/** Append the symbols of an encoded frame, one per dibit. */
void appendFrame(std::vector<int8_t> &symbols, const M17::frame_t &frame);

/** A 40 ms preamble, then the symbols of the frames. */
std::vector<int8_t> m17Symbols(const std::vector<M17::frame_t> &frames);

/** Another 4FSK mode at the M17 symbol rate: never forms an M17 syncword. */
std::vector<int8_t> otherSymbols(const uint32_t seed, const double seconds);

/**
 * LSF, stream frames with distinct payloads and, if eot is set, the
 * last-frame flag and an EOT frame.
 */
std::vector<M17::frame_t> voiceFrames(const size_t numFrames,
                                      const bool eot = true);

/** LSF, packet frames and EOT frame of an SMS filling numFrames frames. */
std::vector<M17::frame_t> smsFrames(const size_t numFrames, const char fill);

} // namespace M17Signal

/**
 * FM discriminator output at 48 kHz: symbols at the level of the reference
 * recording, noise 3x their rms when idle and 30 dB below them otherwise.
 */
class M17Channel : public Channel
{
public:
    static constexpr uint32_t RATE = 48000;
    static constexpr size_t SAMPLES_PER_SYMBOL = RATE / M17::SYMBOL_RATE;

    explicit M17Channel(const uint32_t seed);

    using Channel::transmit;

    /**
     * Append RRC-shaped symbols, the first on a sample equal to phase modulo
     * SAMPLES_PER_SYMBOL. Even phases are whole 24 kHz samples.
     */
    void transmit(const std::vector<int8_t> &symbols, const size_t phase);

private:
    static constexpr float SYMBOL_GAIN = 3000.0f;
    static constexpr float IDLE_RMS = 6000.0f;
    static constexpr float CARRIER_RMS = 70.0f;
};

#endif // M17SIGNAL_H
