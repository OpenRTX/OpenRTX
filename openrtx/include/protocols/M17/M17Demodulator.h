/***************************************************************************
 *   Copyright (C) 2021 - 2022 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN                      *
 *                                Wojciech Kaczmarski SP5WWP               *
 *                                Frederik Saraci IU2NRO                   *
 *                                Silvano Seva IU2KWO                      *
 *                                                                         *
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

#ifndef M17_DEMODULATOR_H
#define M17_DEMODULATOR_H

#ifndef __cplusplus
#error This header is C++ only!
#endif

#include <array>
#include <cstdint>
#include <cstddef>
#include <interfaces/audio_stream.h>
#include <hwconfig.h>
#include <dsp.h>
#include <deque>
#include "M17Datatypes.h"

namespace M17
{

typedef struct
{
    int32_t index;
    bool lsf;
}
sync_t;

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
     * @return true if the last decoded frame is an LSF.
     */
    bool isFrameLSF();

    /**
     * Demodulates data from the ADC and fills the idle frame.
     * Everytime this function is called a whole ADC buffer is consumed.
     *
     * @return true if a new frame has been fully decoded.
     */
    bool update();

    /**
     * @return true if a demodulator is locked on an M17 stream.
     */
    bool isLocked();

private:

    /**
     * M17 baseband signal sampled at 48kHz, half of an M17 frame is processed
     * at each update of the demodulator.
     */
    static constexpr size_t M17_SYMBOL_RATE        = 4800;
    static constexpr size_t M17_FRAME_SYMBOLS      = 192;
    static constexpr size_t M17_SYNCWORD_SYMBOLS   = 8;
    static constexpr size_t M17_RX_SAMPLE_RATE     = 24000;

    static constexpr size_t M17_SAMPLES_PER_SYMBOL = M17_RX_SAMPLE_RATE / M17_SYMBOL_RATE;
    static constexpr size_t M17_FRAME_BYTES        = M17_FRAME_SYMBOLS / 4;
    static constexpr size_t M17_FRAME_SAMPLES      = M17_FRAME_SYMBOLS * M17_SAMPLES_PER_SYMBOL;
    static constexpr size_t M17_SAMPLE_BUF_SIZE    = M17_FRAME_SAMPLES / 2;
    static constexpr size_t M17_BRIDGE_SIZE        = M17_SAMPLES_PER_SYMBOL * M17_SYNCWORD_SYMBOLS;

    static constexpr float  CONV_STATS_ALPHA       = 0.005f;
    static constexpr float  CONV_THRESHOLD_FACTOR  = 3.40;
    static constexpr int16_t QNT_SMA_WINDOW        = 8;

    /**
     * M17 syncwords;
     */
    int8_t  lsf_syncword[M17_SYNCWORD_SYMBOLS]    = { +3, +3, +3, +3, -3, -3, +3, -3 };
    int8_t  stream_syncword[M17_SYNCWORD_SYMBOLS] = { -3, -3, -3, -3, +3, +3, -3, +3 };
    uint8_t lsf_syncword_bytes[2]                 = {0x55, 0xf7};
    uint8_t stream_syncword_bytes[2]              = {0xff, 0x5d};

    /*
     * Buffers
     */
    streamId     basebandId;       ///< Id of the baseband input stream.
    int16_t      *baseband_buffer; ///< Buffer for baseband audio handling.
    dataBlock_t  baseband;         ///< Half buffer, free to be processed.
    uint16_t     *rawFrame;        ///< Analog values to be quantized.
    uint16_t     frame_index;      ///< Index for filling the raw frame.
    frame_t      *activeFrame;     ///< Half frame, in demodulation.
    frame_t      *idleFrame;       ///< Half frame, free to be processed.
    bool         syncDetected;     ///< A syncword was detected.
    bool         locked;           ///< A syncword was correctly demodulated.
    bool         newFrame;         ///< A new frame has been fully decoded.
    int16_t      basebandBridge[M17_BRIDGE_SIZE] = { 0 }; ///< Bridge buffer
    uint16_t     phase;            ///< Phase of the signal w.r.t. sampling

    /*
     * State variables
     */
    bool         m17RxEnabled;     ///< M17 Reception Enabled

    /*
     * Convolution statistics computation
     */
    float conv_emvar = 0.0f;

    /*
     * Quantization statistics computation
     */
    std::deque<int16_t> qnt_pos_fifo;
    std::deque<int16_t> qnt_neg_fifo;
    float qnt_pos_avg = 0.0f;   ///< Rolling average of positive samples
    float qnt_neg_avg = 0.0f;   ///< Rolling average of negative samples

    /*
     * DSP filter state
     */
    filter_state_t dsp_state;

    /**
     * Resets the exponential mean and variance/stddev computation.
     */
    void resetCorrelationStats();

    /**
     * Updates the mean and variance with the given correlation value.
     *
     * @param value: value to be added to the exponential moving
     * average/variance computation
     */
    void updateCorrelationStats(int32_t value);

    /**
     * Returns the standard deviation from all the correlation values.
     *
     * @returns float numerical value of the standard deviation
     */
    float getCorrelationStddev();

    /**
     * Resets the quantization max, min and ema computation.
     */
    void resetQuantizationStats();

    /**
     * Updates the max, min and ema for the received samples.
     *
     * @param offset: index value to be added to the exponential moving
     * average/variance computation
     */
    void updateQuantizationStats(int32_t offset);

    /**
     * Computes the convolution between a stride of samples starting from
     * a given offset and a target waveform.
     *
     * @param offset: the offset in the active buffer where to start the stride
     * @param target: a buffer containing the target waveform to be convoluted
     * @param target_size: the number of symbols of the target waveform
     * @return uint16_t numerical value of the convolution
     */
    int32_t convolution(int32_t offset, int8_t *target, size_t target_size);

    /**
     * Finds the index of the next frame syncword in the baseband stream.
     *
     * @param baseband: buffer containing the sampled baseband signal
     * @param offset: offset of the buffer after which syncword are searched
     * @return uint16_t index of the first syncword in the buffer after the offset
     */
    sync_t nextFrameSync(int32_t offset);

    /**
     * Takes the value from the input baseband at a given offsets and quantizes
     * it leveraging the quantization max and min hold statistics.
     *
     * @param offset: the offset in the input baseband
     * @return int8_t quantized symbol
     */
    int8_t quantize(int32_t offset);

    /**
     * Compute Hamming Distance between two bytes
     */
    uint8_t hammingDistance(uint8_t x, uint8_t y);

};

} /* M17 */

#endif /* M17_DEMODULATOR_H */
