/***************************************************************************
 *   Copyright (C) 2021 - 2022 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN                      *
 *                                Frederik Saraci IU2NRO                   *
 *                                Silvano Seva IU2KWO                      *
 *                                                                         *
 *   Adapted from original code written by Phil Karn KA9Q                  *
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

#ifndef M17_VITERBI_H
#define M17_VITERBI_H

#ifndef __cplusplus
#error This header is C++ only!
#endif

#include <cstdint>
#include <cstddef>
#include <array>
#include <bitset>
#include "M17Utils.hpp"

namespace M17
{

/**
 * Hard decision Viterbi decoder tailored on M17 protocol specifications,
 * that is for decoding of data encoded with a convolutional encoder with a
 * coder rate R = 1/2, a constraint length K = 5 and polynomials G1 = 0x19 and
 * G2 = 0x17.
 */

class M17Viterbi
{
public:

    /**
     * Constructor.
     */
    M17Viterbi() : prevMetrics(&prevMetricsData), currMetrics(&currMetricsData)
    { }

    /**
     * Destructor.
     */
    ~M17Viterbi() { }

    /**
     * Decode unpunctured convolutionally encoded data.
     *
     * @param in: input data.
     * @param out: destination array where decoded data are written.
     * @return number of bit errors corrected.
     */
    template < size_t IN, size_t OUT >
    uint16_t decode(const std::array< uint8_t, IN  >& in,
                          std::array< uint8_t, OUT >& out)
    {
        static_assert(IN*4 < 244, "Input size exceeds max history");

        currMetricsData.fill(0x00);
        prevMetricsData.fill(0x00);

        size_t pos = 0;
        for (size_t i = 0; i < IN*8; i += 2)
        {
            uint8_t s0 = getBit(in, i)     ? 2 : 0;
            uint8_t s1 = getBit(in, i + 1) ? 2 : 0;

            decodeBit(s0, s1, pos);
            pos++;
        }

        return chainback(out, pos) / ((K - 1) >> 1);
    }

    /**
     * Decode punctured convolutionally encoded data.
     *
     * @param in: input data.
     * @param out: destination array where decoded data are written.
     * @return number of bit errors corrected.
     */
    template < size_t IN, size_t OUT, size_t P >
    uint16_t decodePunctured(const std::array< uint8_t, IN  >& in,
                                   std::array< uint8_t, OUT >& out,
                             const std::array< uint8_t, P   >& punctureMatrix)
    {
        static_assert(IN*4 < 244, "Input size exceeds max history");

        currMetricsData.fill(0x00);
        prevMetricsData.fill(0x00);

        size_t   histPos     = 0;
        size_t   punctIndex  = 0;
        size_t   bitPos      = 0;
        uint16_t punctBitCnt = 0;

        while(bitPos < IN*8)
        {
            uint8_t sym[2] = {1, 1};

            for(uint8_t i = 0; i < 2; i++)
            {
                if(punctureMatrix[punctIndex++])
                {
                    sym[i] = getBit(in, bitPos) ? 2 : 0;
                    bitPos++;
                }
                else
                {
                    punctBitCnt++;
                }

                if(punctIndex >= P) punctIndex = 0;
            }

            decodeBit(sym[0], sym[1], histPos);
            histPos++;
        }

        return (chainback(out, histPos) - punctBitCnt) / ((K - 1) >> 1);
    }

private:

    /**
     * Decode one bit and update trellis.
     *
     * @param s0: cost of the first symbol.
     * @param s1: cost of the second symbol.
     * @param pos: bit position in history.
     */
    void decodeBit(uint8_t s0, uint8_t s1, size_t pos)
    {
        static constexpr uint8_t COST_TABLE_0[] = {0, 0, 0, 0, 2, 2, 2, 2};
        static constexpr uint8_t COST_TABLE_1[] = {0, 2, 2, 0, 0, 2, 2, 0};

        for(uint8_t i = 0; i < NumStates/2; i++)
        {
            uint16_t metric = std::abs(COST_TABLE_0[i] - s0)
                            + std::abs(COST_TABLE_1[i] - s1);


            uint16_t m0 = (*prevMetrics)[i] + metric;
            uint16_t m1 = (*prevMetrics)[i + NumStates/2] + (4 - metric);

            uint16_t m2 = (*prevMetrics)[i] + (4 - metric);
            uint16_t m3 = (*prevMetrics)[i + NumStates/2] + metric;

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
     * @return minimum Viterbi cost at the end of the decode sequence.
     */
    template < size_t OUT >
    uint16_t chainback(std::array< uint8_t, OUT >& out, size_t pos)
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

        uint16_t cost = (*prevMetrics)[0];

        for(size_t i = 0; i < NumStates; i++)
        {
            uint16_t m = (*prevMetrics)[i];
            if(m < cost) cost = m;
        }

        return cost;
    }


    static constexpr size_t K = 5;
    static constexpr size_t NumStates = (1 << (K - 1));

    std::array< uint16_t, NumStates > *prevMetrics;
    std::array< uint16_t, NumStates > *currMetrics;

    std::array< uint16_t, NumStates >  prevMetricsData;
    std::array< uint16_t, NumStates >  currMetricsData;

    std::array< std::bitset< NumStates >, 244 > history;
};

}      // namespace M17

#endif // M17_VITERBI_H
