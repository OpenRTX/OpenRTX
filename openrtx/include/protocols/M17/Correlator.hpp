/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CORRELATOR_H
#define CORRELATOR_H

#include <cstdint>
#include <array>

/**
 * Class to construct correlator objects, allowing to compute the cross-correlation
 * between a stream of signed 16-bit samples and a known syncword.
 * The correlator has its internal storage for past samples.
 */
template < size_t SYNCW_SIZE, size_t SAMPLES_PER_SYM >
class Correlator
{
public:

    /**
     * Constructor.
     */
    Correlator() : sampIdx(0) { }

    /**
     * Destructor.
     */
    ~Correlator() { }

    /**
     * Append a new sample to the correlator memory.
     *
     * @param sample: baseband sample.
     */
    void sample(const int16_t sample)
    {
        samples[sampIdx] = sample;
        prevIdx = sampIdx;
        sampIdx = (sampIdx + 1) % SYNCWORD_SAMPLES;
    }

    /**
     * Compute the convolution product between the samples stored in the correlator
     * memory and a target syncword.
     *
     * @param syncword: syncword symbols.
     * @return convolution product.
     */
    int32_t convolve(const std::array< int8_t, SYNCW_SIZE >& syncword)
    {
        int32_t conv = 0;
        size_t  pos  = prevIdx + SAMPLES_PER_SYM;

        for(auto& sym : syncword)
        {
            conv += (int32_t) sym * (int32_t) samples[pos % SYNCWORD_SAMPLES];
            pos  += SAMPLES_PER_SYM;
        }

        return conv;
    }

    /**
     * Return the maximum deviation of the samples stored in the correlator
     * memory, starting from a given sampling point. When the sampling point
     * corresponds to a peak of correlation, this function allows to retrieve
     * the outer deviation of a given baseband stream, provided that the target
     * syncword is composed only by outer symbols. This is true in case the
     * syncword is constructed using Barker codes.
     *
     * @param samplePoint: sampling point.
     * @return a std::pair carrying the maximum deviation. First element is
     * positive deviation, second element is negative deviation.
     */
    std::pair< int32_t, int32_t > maxDeviation(const uint8_t samplePoint)
    {
        int32_t maxSum = 0;
        int32_t minSum = 0;
        int32_t maxCnt = 0;
        int32_t minCnt = 0;

        for(size_t i = 0; i < SYNCWORD_SAMPLES; i++)
        {
            if(((prevIdx + i) % SAMPLES_PER_SYM) == samplePoint)
            {
                int16_t sample = samples[(prevIdx + i) % SYNCWORD_SAMPLES];
                if(sample > 0)
                {
                    maxSum += sample;
                    maxCnt += 1;
                }

                if(sample < 0)
                {
                    minSum += sample;
                    minCnt += 1;
                }
            }
        }

        if((maxCnt == 0) || (minCnt == 0))
            return std::make_pair(0, 0);

        return std::make_pair(maxSum/maxCnt, minSum/minCnt);
    }

    /**
     * Access the internal sample memory.
     *
     * @return a pointer to the correlator memory.
     */
    const int16_t *data()
    {
        return samples;
    }

    /**
     * Get the buffer index at which the last sample has been written. The index
     * goes from zero to (SYNCW_SIZE * SAMPLES_PER_SYM) - 1.
     *
     * @return index of the last stored sample.
     */
    size_t index()
    {
        return prevIdx;
    }

    /**
     * Get the index at which the last sample has been written, modulo the
     * number of samples a symbol is made of.
     *
     * @return index of the last stored sample.
     */
    size_t sampleIndex()
    {
        return prevIdx % SAMPLES_PER_SYM;
    }

private:

    static constexpr size_t SYNCWORD_SAMPLES = SYNCW_SIZE * SAMPLES_PER_SYM;

    int16_t samples[SYNCWORD_SAMPLES];  ///< Samples' storage
    size_t  sampIdx;                    ///< Index of the next sample to write
    size_t  prevIdx;                    ///< Index of the last written sample
};

#endif
