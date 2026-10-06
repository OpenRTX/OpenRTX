/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef VITERBI_H
#define VITERBI_H

#ifndef __cplusplus
#error This header is C++ only!
#endif

#include <cstdint>
#include <cstddef>
#include <array>
#include <bitset>
#include "Utils.hpp"

namespace M17
{

/**
 * Soft decision Viterbi decoder tailored on M17 protocol specifications,
 * that is for decoding of data encoded with a convolutional encoder with a
 * coder rate R = 1/2, a constraint length K = 5 and polynomials G1 = 0x19 and
 * G2 = 0x17.
 */

class SoftViterbi
{
public:

    /**
     * Constructor.
     */
    SoftViterbi() : prevMetrics(&prevMetricsData), currMetrics(&currMetricsData)
    { }

    /**
     * Destructor.
     */
    ~SoftViterbi() { }

    /**
     * Decode unpunctured convolutionally encoded data.
     *
     * @param in: input data.
     * @param out: destination array where decoded data are written.
     */
    template < size_t IN, size_t OUT >
    void decode(const std::array< uint16_t, IN >& in,
                      std::array< uint8_t, OUT >& out)
    {
        static_assert(IN < 244*2, "Input size exceeds max history");

        currMetricsData.fill(0);
        prevMetricsData.fill(0);

        size_t pos = 0;
        for (size_t i = 0; i < IN; i += 2)
        {
            uint16_t s0 = in[i];
            uint16_t s1 = in[i + 1];

            decodeBit(s0, s1, pos);
            pos++;
        }

        chainback(out, pos);
    }

    /**
     * Decode punctured convolutionally encoded data.
     *
     * @param in: input data.
     * @param out: destination array where decoded data are written.
     * @param punctureMatrix: puncturing matrix.
     */
    template < size_t IN, size_t OUT, size_t P >
    void decodePunctured(const std::array< uint16_t, IN >& in,
                               std::array< uint8_t, OUT >& out,
                         const std::array< uint8_t, P   >& punctureMatrix)
    {
        static_assert(IN < 244*2, "Input size exceeds max history");

        currMetricsData.fill(0);
        prevMetricsData.fill(0);

        size_t histPos    = 0;
        size_t punctIndex = 0;
        size_t bitPos     = 0;

        while(bitPos < IN)
        {
            uint16_t sym[2] = {0xFFFF, 0xFFFF};

            for(uint8_t i = 0; i < 2; i++)
            {
                if(punctureMatrix[punctIndex++])
                {
                    sym[i] = in[bitPos];
                    bitPos++;
                }
                else
                {
                    sym[i] = 0x7FFF; //half range for punctured out bit
                }

                if(punctIndex >= P) punctIndex = 0;
            }

            decodeBit(sym[0], sym[1], histPos);
            histPos++;
        }

        chainback(out, histPos);
    }

private:

    /**
     * Decode one bit and update trellis.
     *
     * @param s0: cost of the first symbol.
     * @param s1: cost of the second symbol.
     * @param pos: bit position in history.
     */
    void decodeBit(uint16_t s0, uint16_t s1, size_t pos)
    {
        static constexpr uint16_t COST_TABLE_0[] = {0, 0, 0, 0, 0xFFFF,
                                                    0xFFFF, 0xFFFF, 0xFFFF};
        static constexpr uint16_t COST_TABLE_1[] = {0, 0xFFFF, 0xFFFF, 0,
                                                    0, 0xFFFF, 0xFFFF, 0};

        for(uint8_t i = 0; i < NumStates/2; i++)
        {
            uint32_t metric = q_AbsDiff(COST_TABLE_0[i], s0)
                            + q_AbsDiff(COST_TABLE_1[i], s1);


            uint32_t m0 = (*prevMetrics)[i] + metric;
            uint32_t m1 = (*prevMetrics)[i + NumStates/2] + (0x1FFFE - metric);

            uint32_t m2 = (*prevMetrics)[i] + (0x1FFFE - metric);
            uint32_t m3 = (*prevMetrics)[i + NumStates/2] + metric;

            uint8_t i0 = 2 * i;
            uint8_t i1 = i0 + 1;

            if(m0 >= m1)
            {
                history[pos].set(i0, true);
                (*currMetrics)[i0] = m1;
            }
            else
            {
                history[pos].set(i0, false);
                (*currMetrics)[i0] = m0;
            }

            if(m2 >= m3)
            {
                history[pos].set(i1, true);
                (*currMetrics)[i1] = m3;
            }
            else
            {
                history[pos].set(i1, false);
                (*currMetrics)[i1] = m2;
            }
        }

        std::swap(currMetrics, prevMetrics);
    }

    /**
     * History chainback to obtain final byte array.
     *
     * @param out: destination byte array for decoded data.
     * @param pos: starting position for the chainback.
     */
    template < size_t OUT >
    void chainback(std::array< uint8_t, OUT >& out, size_t pos)
    {
        uint8_t state = 0;
        size_t bitPos = OUT*8;

        while(bitPos > 0)
        {
            bitPos--;
            pos--;
            bool bit = history[pos].test(state >> 4);
            state >>= 1;
            if(bit) state |= 0x80;
            setBit(out, bitPos, bit);
        }
    }

    /**
     * Utility function to compute the absolute value of a difference between
     * two fixed-point values.
     *
     * @param v1: first value
     * @param v2: second value
     * @return abs(v1-v2)
     */
    inline uint16_t q_AbsDiff(const uint16_t v1, const uint16_t v2)
    {
        if(v2 > v1) return v2 - v1;
        return v1 - v2;
    }


    static constexpr size_t K = 5;
    static constexpr size_t NumStates = (1 << (K - 1));

    std::array< uint32_t, NumStates > *prevMetrics;
    std::array< uint32_t, NumStates > *currMetrics;

    std::array< uint32_t, NumStates >  prevMetricsData;
    std::array< uint32_t, NumStates >  currMetricsData;

    std::array< std::bitset< NumStates >, 244 > history;
};

}      // namespace M17

#endif // VITERBI_H
