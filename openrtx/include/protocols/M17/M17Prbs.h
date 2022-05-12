/***************************************************************************
 *   Copyright (C) 2022 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
 *                         Frederik Saraci IU2NRO                          *
 *                         Silvano Seva IU2KWO                             *
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

#ifndef M17PRBS_H
#define M17PRBS_H

#ifndef __cplusplus
#error This header is C++ only!
#endif

#include <cstdint>

namespace M17
{

/**
 * Pseudo random number generator compliant with M17 PRBS9 specification.
 */
class PRBS9
{
public:

    /**
     * Constructor.
     */
    PRBS9() : state(1), syncCnt(0), synced(false) { }

    /**
     * Destructor.
     */
    ~PRBS9() { }

    /**
     * Reset the generator state.
     */
    void reset()
    {
        state   = 1;
        syncCnt = 0;
        synced  = false;
    }

    /**
     * Generate a new bit from the PRBS9 sequence.
     *
     * @return a bit from the PRBS9 sequence.
     */
    bool generateBit()
    {
        bool result = ((state >> TAP_1) ^ (state >> TAP_2)) & 0x01;
        state       = ((state << 1) | result) & MASK;
        return result;
    }

    /**
     * Syncronise the PRBS9 generator with an external stream of bits.
     *
     * @param bit: current bit of the external stream.
     * @return true when the PRBS9 state is syncronised with the bit stream.
     */
    bool syncronize(const bool& bit)
    {
        if(synced) return true;

        bool result = (bit ^ (state >> TAP_1) ^ (state >> TAP_2)) & 1;
        state       = ((state << 1) | bit) & MASK;

        syncCnt += 1;

        if(result)
        {
            syncCnt = 0;
            synced  = false;
        }

        if(syncCnt >= LOCK_COUNT)
        {
            synced = true;
        }

        return synced;
    }

    /**
     * Validate the current bit of an external stream against the PRBS9 state
     * and advance the generator state by one step. To have meaningful results
     * the generator state has to be syncronised with the stream.
     *
     * @param bit: current bit of the external stream.
     * @return true if the bit from the PRBS9 matches the external one, false
     * otherwise.
     */
    bool validateBit(const bool& bit)
    {
        if(synced == false) return false;
        return ((bit ^ generateBit()) == 0);
    }

private:

    static constexpr uint16_t MASK       = 0x1FF;
    static constexpr uint8_t  TAP_1      = 8;
    static constexpr uint8_t  TAP_2      = 4;
    static constexpr uint8_t  LOCK_COUNT = 18;

    uint16_t state;
    uint8_t  syncCnt;
    bool     synced;
};

}      // namespace M17

#endif // M17PRBS_H
