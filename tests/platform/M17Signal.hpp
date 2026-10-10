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
#include "NoisyChannel.hpp"

/**
 * M17 test signals: frames encoded by FrameEncoder, their symbols, and the
 * baseband reaching the demodulator.
 */
namespace M17Signal
{

/**
 * RRC-shaped 24 kHz baseband fed straight to the demodulator.
 *
 * @param symbols: symbols to shape.
 * @param amplitude: amplitude of a unit symbol.
 * @return the baseband samples.
 */
std::vector<int16_t> rrcBaseband(const std::vector<int8_t> &symbols,
                                 const float amplitude = 2000.0f);

/**
 * Append preamble symbols, alternating +3 and -3.
 *
 * @param symbols: symbols to append to.
 * @param count: number of preamble symbols.
 */
void appendPreamble(std::vector<int8_t> &symbols,
                    const size_t count = M17::FRAME_SYMBOLS);

/**
 * Append the symbols of an encoded frame, one per dibit.
 *
 * @param symbols: symbols to append to.
 * @param frame: encoded frame.
 */
void appendFrame(std::vector<int8_t> &symbols, const M17::frame_t &frame);

/**
 * Symbols of a transmission: a 40 ms preamble, then the frames.
 *
 * @param frames: encoded frames.
 * @return the symbols.
 */
std::vector<int8_t> m17Symbols(const std::vector<M17::frame_t> &frames);

/**
 * Symbols of another 4FSK mode at the M17 symbol rate, never forming an M17
 * syncword.
 *
 * @param seed: symbol generator seed.
 * @param seconds: duration.
 * @return the symbols.
 */
std::vector<int8_t> otherSymbols(const uint32_t seed, const double seconds);

/**
 * Frames of a voice transmission: LSF and stream frames with distinct
 * payloads.
 *
 * @param numFrames: number of stream frames.
 * @param eot: end with the last-frame flag and an EOT frame.
 * @return the frames.
 */
std::vector<M17::frame_t> voiceFrames(const size_t numFrames,
                                      const bool eot = true);

/**
 * Frames of an SMS: LSF, packet frames and EOT frame.
 *
 * @param numFrames: number of packet frames the SMS fills.
 * @param fill: character the text is made of.
 * @return the frames.
 */
std::vector<M17::frame_t> smsFrames(const size_t numFrames, const char fill);

} // namespace M17Signal

/**
 * FM discriminator output at 48 kHz: symbols at the level of the reference
 * recording, noise 3x their rms when idle and 30 dB below them otherwise.
 */
class M17NoisyChannel : public NoisyChannel
{
public:
    static constexpr uint32_t RATE = 48000; ///< Sample rate, in Hz.
    /** Samples per symbol. */
    static constexpr size_t SAMPLES_PER_SYMBOL = RATE / M17::SYMBOL_RATE;

    /**
     * Constructor.
     *
     * @param seed: noise generator seed.
     */
    explicit M17NoisyChannel(const uint32_t seed);

    using NoisyChannel::transmit;

    /**
     * Append RRC-shaped symbols with their noise.
     *
     * @param symbols: symbols to transmit.
     * @param phase: sample of the first symbol, modulo SAMPLES_PER_SYMBOL.
     * Even phases are whole 24 kHz samples.
     */
    void transmit(const std::vector<int8_t> &symbols, const size_t phase);

private:
    static constexpr float SYMBOL_GAIN = 3000.0f; ///< Unit symbol amplitude.
    static constexpr float IDLE_RMS = 6000.0f;    ///< Noise rms when idle.
    static constexpr float CARRIER_RMS = 70.0f;   ///< Noise rms on a carrier.
};

#endif // M17SIGNAL_H
