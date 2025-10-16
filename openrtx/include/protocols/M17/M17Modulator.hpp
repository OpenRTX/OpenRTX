/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef M17_MODULATOR_H
#define M17_MODULATOR_H

#ifndef __cplusplus
#error This header is C++ only!
#endif

#include "core/audio_stream.h"
#include "protocols/M17/PwmCompensator.hpp"
#include "protocols/M17/M17Constants.hpp"
#include "core/audio_path.h"
#include <cstdint>
#include <memory>
#include <array>

namespace M17
{

/**
 * Modulator device for M17 protocol.
 */
class M17Modulator
{
public:

    /**
     * Constructor.
     */
    M17Modulator();

    /**
     * Destructor.
     */
    ~M17Modulator();

    /**
     * Allocate buffers for baseband audio generation and initialise modulator.
     */
    void init();

    /**
     * Forcefully shutdown modulator and deallocate data buffers.
     */
    void terminate();

    /**
     * Start the modulator.
     *
     * @return true if the modulator has been successfully started.
     */
    bool start();

    /**
     * Send an 80ms long transmission preamble.
     */
    void sendPreamble();

    /**
     * Generate and transmit the baseband signal obtained by 4FSK modulation of
     * a given block of data.
     *
     * @param frame: M17 frame to be sent.
     * @param isLast: flag signalling that current block is the last one being
     * transmitted.
     */
    void sendFrame(const frame_t& frame);

    /**
     * Terminate baseband transmission.
     */
    void stop();

    /**
     * Invert baseband signal phase before output.
     *
     * @param status: if set to true signal phase is inverted.
     */
    void invertPhase(const bool status);

private:

    /**
     * Generate baseband stream from symbol stream.
     */
    void symbolsToBaseband();

    /**
     * Emit the baseband stream towards the output stage, platform dependent.
     */
    void sendBaseband();

    static constexpr size_t M17_TX_SAMPLE_RATE     = 48000;
    static constexpr size_t M17_SAMPLES_PER_SYMBOL = M17_TX_SAMPLE_RATE / M17_SYMBOL_RATE;
    static constexpr size_t M17_FRAME_SAMPLES      = M17_FRAME_SYMBOLS * M17_SAMPLES_PER_SYMBOL;

    static constexpr float  M17_RRC_GAIN          = 23000.0f;
    static constexpr float  M17_RRC_OFFSET        = 0.0f;

    std::array< int8_t, M17_FRAME_SYMBOLS > symbols;
    std::unique_ptr< int16_t[] > baseband_buffer;  ///< Buffer for baseband audio handling.
    stream_sample_t              *idleBuffer;      ///< Half baseband buffer, free for processing.
    streamId                     outStream;        ///< Baseband output stream ID.
    pathId                       outPath;          ///< Baseband output path ID.
    bool                         txRunning;        ///< Transmission running.
    bool                         invPhase;        ///< Invert signal phase

    #if defined(PLATFORM_MD3x0) || defined(PLATFORM_MDUV3x0)
    PwmCompensator pwmComp;
    #endif
};

} /* M17 */

#endif /* M17_MODULATOR_H */
