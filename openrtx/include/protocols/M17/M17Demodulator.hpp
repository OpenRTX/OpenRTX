/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef M17_DEMODULATOR_H
#define M17_DEMODULATOR_H

#ifndef __cplusplus
#error This header is C++ only!
#endif

#include "core/iir.hpp"
#include <cstdint>
#include <cstddef>
#include <memory>
#include <array>
#include "core/dsp.h"
#include <cmath>
#include "core/audio_path.h"
#include "core/audio_stream.h"
#include "protocols/M17/M17Datatypes.hpp"
#include "protocols/M17/M17Constants.hpp"
#include "protocols/M17/Correlator.hpp"
#include "protocols/M17/Synchronizer.hpp"
#include "protocols/M17/DevEstimator.hpp"
#include "protocols/M17/ClockRecovery.hpp"

namespace M17
{

class M17Demodulator
{
public:

    /**
     * Constructor.
     */
    M17Demodulator();

    /**
     * Destructor.
     */
    ~M17Demodulator();

    /**
     * Allocate buffers for baseband signal sampling and initialise demodulator.
     */
    void init();

    /**
     * Shutdown modulator and deallocate data buffers.
     */
    void terminate();

    /**
     * Starts the sampling of the baseband signal in a double buffer.
     */
    void startBasebandSampling();

    /**
     * Stops the sampling of the baseband signal in a double buffer.
     */
    void stopBasebandSampling();

    /**
     * Returns the a frame decoded from the baseband signal.
     *
     * @return reference to the internal data structure containing the last
     * decoded frame.
     */
    const frame_t& getFrame();

    /**
     * Demodulates data from the ADC and fills the idle frame.
     * Everytime this function is called a whole ADC buffer is consumed.
     *
     * @param invertPhase: invert the phase of the baseband signal before decoding.
     * @return true if a new frame has been fully decoded.
     */
    bool update(const bool invertPhase = false);

    /**
     * @return true if a demodulator is locked on an M17 stream.
     */
    bool isLocked();

private:

    /**
     * Quantize a given sample to its corresponding symbol and append it to the
     * ongoing frame. When a frame is complete, it swaps the pointers and updates
     * newFrame variable.
     *
     * @param sample: baseband sample.
     * @return quantized symbol.
     */
    void quantize(const int16_t sample);

    /**
     * Reset the demodulator state.
     */
    void reset();

    /**
     * State handler function for DemodState::UNLOCKED
     */
    void unlockedState();

    /**
     * State handler function for DemodState::SYNCED
     */
    void syncedState();

    /**
     * State handler function for DemodState::LOCKED
     *
     * @param sample: current baseband sample
     */
    void lockedState(int16_t sample);

    /**
     * State handler function for DemodState::SYNC_UPDATE
     */
    void syncUpdateState();

    /**
     * M17 baseband signal sampled at 24kHz, half of an M17 frame is processed
     * at each update of the demodulator.
     */
    static constexpr size_t  RX_SAMPLE_RATE     = 24000;
    static constexpr size_t  SAMPLES_PER_SYMBOL = RX_SAMPLE_RATE / M17_SYMBOL_RATE;
    static constexpr size_t  FRAME_SAMPLES      = M17_FRAME_SYMBOLS * SAMPLES_PER_SYMBOL;
    static constexpr size_t  SAMPLE_BUF_SIZE    = FRAME_SAMPLES / 2;
    static constexpr size_t  SYNCWORD_SAMPLES   = SAMPLES_PER_SYMBOL * M17_SYNCWORD_SYMBOLS;

    /**
     * Internal state of the demodulator.
     */
    enum class DemodState
    {
        INIT,       ///< Initializing
        UNLOCKED,   ///< Not locked
        SYNCED,     ///< Synchronized, validate syncword
        LOCKED,     ///< Locked
        SYNC_UPDATE ///< Updating the synchronization state
    };

    /**
     * Cofficients of the sample filter
     */
    static constexpr std::array < float, 3 > sfNum = {4.24433681e-05f, 8.48867363e-05f, 4.24433681e-05f};
    static constexpr std::array < float, 3 > sfDen = {1.0f,           -1.98148851f,     0.98165828f};

    DemodState                     demodState;      ///< Demodulator state
    std::unique_ptr< int16_t[] >   baseband_buffer; ///< Buffer for baseband audio handling.
    streamId                       basebandId;      ///< Id of the baseband input stream.
    pathId                         basebandPath;    ///< Id of the baseband input path.
    std::unique_ptr<frame_t >      demodFrame;      ///< Frame being demodulated.
    std::unique_ptr<frame_t >      readyFrame;      ///< Fully demodulated frame to be returned.
    bool                           newFrame;        ///< A new frame has been fully decoded.
    bool                           resetClockRec;   ///< Clock recovery reset request.
    bool                           updateSampPoint; ///< Sampling point update pending.
    uint16_t                       frameIndex;      ///< Index for filling the raw frame.
    uint32_t                       sampleIndex;     ///< Sample index, from 0 to (SAMPLES_PER_SYMBOL - 1)
    uint32_t                       samplingPoint;   ///< Symbol sampling point
    uint32_t                       sampleCount;     ///< Free-running sample counter
    uint8_t                        missedSyncs;     ///< Counter of missed synchronizations
    uint32_t                       initCount;       ///< Downcounter for initialization
    float                          corrThreshold;   ///< Correlation threshold
    struct dcBlock                 dcBlock;         ///< State of the DC removal filter

    Correlator   < M17_SYNCWORD_SYMBOLS, SAMPLES_PER_SYMBOL > correlator;
    Synchronizer < M17_SYNCWORD_SYMBOLS, SAMPLES_PER_SYMBOL > streamSync{{ -3, -3, -3, -3, +3, +3, -3, +3 }};
    Iir          < 3 >                                        sampleFilter{sfNum, sfDen};
    DevEstimator                                              devEstimator;
    ClockRecovery< SAMPLES_PER_SYMBOL >                       clockRec;
};

} /* M17 */

#endif /* M17_DEMODULATOR_H */
