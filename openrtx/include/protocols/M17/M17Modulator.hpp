/***************************************************************************
 *   Copyright (C) 2021 - 2022 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN                      *
 *                                Frederik Saraci IU2NRO                   *
 *                                Silvano Seva IU2KWO                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#ifndef M17_MODULATOR_H
#define M17_MODULATOR_H

#ifndef __cplusplus
#error This header is C++ only!
#endif

#include <interfaces/audio_stream.h>
#include <M17/M17Constants.hpp>
#include <cstdint>
#include <memory>
#include <array>
#include <dsp.h>

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
     * Generate and transmit the baseband signal obtained by 4FSK modulation of
     * a given block of data. When called for the first time, this function
     * starts baseband transmission.
     *
     * @param sync: synchronisation word to be prepended to data block.
     * @param data: data block to be transmitted.
     * @param isLast: flag signalling that current block is the last one being
     * transmitted.
     */
    void send(const std::array< uint8_t, 2 >& sync,
              const std::array< uint8_t, 46 >& data,
              const bool isLast = false);

private:

    /**
     * Generate baseband stream from symbol stream.
     */
    void generateBaseband();

    /**
     * Emit the baseband stream towards the output stage, platform dependent.
     */
    void emitBaseband();

    /** Gracefully end the transmission **/
    void stop();

    static constexpr size_t M17_TX_SAMPLE_RATE     = 48000;
    static constexpr size_t M17_SAMPLES_PER_SYMBOL = M17_TX_SAMPLE_RATE / M17_SYMBOL_RATE;
    static constexpr size_t M17_FRAME_SAMPLES      = M17_FRAME_SYMBOLS * M17_SAMPLES_PER_SYMBOL;

    #ifdef PLATFORM_MOD17
    static constexpr float  M17_RRC_GAIN          = 15000.0f;
    static constexpr float  M17_RRC_OFFSET        = 11500.0f;
    #else
    static constexpr float  M17_RRC_GAIN          = 23000.0f;
    static constexpr float  M17_RRC_OFFSET        = 0.0f;
    #endif

    std::array< int16_t, M17_FRAME_SYMBOLS > symbols;
    std::unique_ptr< int16_t[] > baseband_buffer;  ///< Buffer for baseband audio handling.
    stream_sample_t              *idleBuffer;      ///< Half baseband buffer, free for processing.
    streamId                     outStream;        ///< Baseband output stream ID.
    bool                         txRunning;        ///< Transmission running.

    #if defined(PLATFORM_MD3x0) || defined(PLATFORM_MDUV3x0)
    filter_state_t   pwmFilterState;
    #endif
};

} /* M17 */

#endif /* M17_MODULATOR_H */
