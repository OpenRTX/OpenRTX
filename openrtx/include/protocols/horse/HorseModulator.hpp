/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef HORSE_MODULATOR_H
#define HORSE_MODULATOR_H

#include "core/audio_stream.h"
#include "HorseConstants.hpp"
#include "core/audio_path.h"
#include <cstdint>
#include <memory>
#include <array>

#if defined(PLATFORM_MD3x0) || defined(PLATFORM_MDUV3x0)
#include "protocols/M17/PwmCompensator.hpp"
#endif

#ifndef __cplusplus
#error This header is C++ only!
#endif

namespace horse
{

class HorseModulator
{
public:
    HorseModulator();
    ~HorseModulator();

    void init();
    void terminate();

    bool start();
    void sendPreamble();
    void sendFrame(const frame_t &frame);
    void stop();

    void invertPhase(bool status);

    static constexpr size_t captureSamplesPerFrame()
    {
        return FRAME_SYMBOLS * (48000 / SYMBOL_RATE);
    }

    void beginCapture(int16_t *buf, size_t cap);
    size_t captureLength() const;
    void endCapture();

private:
    void symbolsToBaseband();
    void sendBaseband();

    static constexpr size_t TX_SAMPLE_RATE = 48000;
    static constexpr size_t SAMPLES_PER_SYMBOL = TX_SAMPLE_RATE / SYMBOL_RATE;
    static constexpr size_t FRAME_SAMPLES = FRAME_SYMBOLS * SAMPLES_PER_SYMBOL;
    static constexpr float RRC_GAIN = 23000.0f;
    static constexpr float RRC_OFFSET = 0.0f;

    std::array<int8_t, FRAME_SYMBOLS> symbols;
    std::unique_ptr<int16_t[]> baseband_buffer;
    stream_sample_t *idleBuffer;
    streamId outStream;
    pathId outPath;
    bool txRunning;
    bool invPhase;
    int16_t *captureBuf;
    size_t captureCap;
    size_t captureLen;

#if defined(PLATFORM_MD3x0) || defined(PLATFORM_MDUV3x0)
    PwmCompensator pwmComp;
#endif
};

} // namespace horse

#endif // HORSE_MODULATOR_H
