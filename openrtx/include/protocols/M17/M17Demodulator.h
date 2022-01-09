/***************************************************************************
 *   Copyright (C) 2021 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
 *                         Wojciech Kaczmarski SP5WWP                      *
 *                         Frederik Saraci IU2NRO                          *
 *                         Silvano Seva IU2KWO                             *
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

namespace M17
{

typedef struct
{
    int32_t index;
    bool lsf;
} sync_t;

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
     */
    const std::array<uint8_t, 48> &getFrame();

    /**
     * Returns a flag indicating whether the decoded frame is an LSF.
     */
    bool isFrameLSF();

    /**
     * Demodulates data from the ADC and fills the idle frame
     * Everytime this function is called a whole ADC buffer is consumed
     */
    void update();

private:

    /*
     * We are sampling @ 24KHz so an M17 frame has half the samples,
     * our input buffer contains half M17 frame.
     */
    // TODO: Select correct RRC filter according to sample rate
    static constexpr size_t M17_RX_SAMPLE_RATE     = 24000;
    static constexpr size_t M17_FRAME_SAMPLES_24K  = 960;
    static constexpr size_t M17_FRAME_SYMBOLS      = 192;
    static constexpr size_t M17_SYNCWORD_SYMBOLS   = 8;
    static constexpr size_t M17_CONV_THRESHOLD     = 50000;
    static constexpr size_t M17_SAMPLES_PER_SYMBOL = M17_SPS;
    static constexpr size_t M17_INPUT_BUF_SIZE     = M17_FRAME_SAMPLES_24K / 2;
    static constexpr size_t M17_FRAME_BYTES        = M17_FRAME_SYMBOLS / 4;
    static constexpr float conv_stats_alpha        = 0.0001f;
    static constexpr float conv_threshold_factor   = 3.70;
    static constexpr float qnt_maxmin_alpha        = 0.999f;
    static constexpr size_t M17_BRIDGE_SIZE        = M17_SAMPLES_PER_SYMBOL *
                                                     M17_SYNCWORD_SYMBOLS;

    /**
     * M17 syncwords;
     */
    int8_t lsf_syncword[M17_SYNCWORD_SYMBOLS]    = { +3, +3, +3, +3, -3, -3, +3, -3 };
    int8_t stream_syncword[M17_SYNCWORD_SYMBOLS] = { -3, -3, -3, -3, +3, +3, -3, +3 };
    uint8_t lsf_syncword_bytes[2]                = {0x55, 0xf7};
    uint8_t stream_syncword_bytes[2]             = {0xff, 0x5d};

    using dataBuffer_t = std::array< int16_t, M17_FRAME_SAMPLES_24K >;
    using dataFrame_t =  std::array< uint8_t, M17_FRAME_BYTES >;

    /*
     * Buffers
     */
    streamId     basebandId;       ///< Id of the baseband input stream.
    int16_t      *baseband_buffer; ///< Buffer for baseband audio handling.
    dataBlock_t  baseband;         ///< Half buffer, free to be processed.
    uint16_t     *rawFrame;        ///< Analog values to be quantized.
    uint16_t     frameIndex;       ///< Index for filling the raw frame.
    dataFrame_t  *activeFrame;     ///< Half frame, in demodulation.
    dataFrame_t  *idleFrame;       ///< Half frame, free to be processed.
    bool         isLSF;            ///< Indicates that we demodualated an LSF.
    bool         locked;           ///< A syncword was detected.
    int16_t      basebandBridge[M17_BRIDGE_SIZE] = { 0 }; ///< Bridge buffer
    uint16_t     phase;            ///< Phase of the signal w.r.t. sampling

    /*
     * State variables
     */
    bool         m17RxEnabled;     ///< M17 Reception Enabled

    /*
     * Convolution statistics computation
     */
    float conv_ema = 0.0f;
    float conv_emvar = 0.0f;

    /*
     * Quantization statistics computation
     */
    float qnt_max = 0.0f;
    float qnt_min = 0.0f;

    /**
     * Resets the exponential mean and variance/stddev computation.
     */
    void resetCorrelationStats();

    /**
     * Updates the mean and variance with the given value.
     *
     * @param value: value to be added to the exponential moving
     * average/variance computation
     */
    void updateCorrelationStats(int32_t value);

    /**
     * Returns the standard deviation from all the passed values.
     *
     * @returns float numerical value of the standard deviation
     */
    float getCorrelationStddev();

    void resetQuantizationStats();

    void updateQuantizationStats(int32_t offset);

    float getQuantizationMax();

    float getQuantizationMin();

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

};

} /* M17 */

#endif /* M17_DEMODULATOR_H */
