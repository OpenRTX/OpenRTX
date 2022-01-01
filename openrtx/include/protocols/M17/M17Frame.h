/***************************************************************************
 *   Copyright (C) 2021 by Federico Amedeo Izzo IU2NUO,                    *
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

#ifndef M17_FRAME_H
#define M17_FRAME_H

#ifndef __cplusplus
#error This header is C++ only!
#endif

#include <cstring>
#include <string>
#include "M17Datatypes.h"

/**
 * This class describes and handles a generic M17 data frame.
 */
class M17Frame
{
public:

    /**
     * Constructor.
     */
    M17Frame()
    {
        clear();
    }

    /**
     * Destructor.
     */
    ~M17Frame(){ }

    /**
     * Clear the frame content, filling it with zeroes.
     */
    void clear()
    {
        memset(&data, 0x00, sizeof(dataFrame_t));
    }

    /**
     * Set frame sequence number.
     *
     * @param seqNum: frame number, between 0 and 0x7FFF.
     */
    void setFrameNumber(const uint16_t seqNum)
    {
        // NOTE: M17 fields are big-endian, we need to swap bytes
        data.frameNum = __builtin_bswap16(seqNum &  0x7fff);
    }

    /**
     * Mark this frame as the last one in the transmission, informing the
     * receiver that transmission ends.
     */
    void lastFrame()
    {
        data.frameNum |= 0x0080;
    }

    /**
     * Access frame payload.
     *
     * @return a reference to frame's paylod field, allowing for both read and
     * write access.
     */
    payload_t& payload()
    {
        return data.payload;
    }

    /**
     * Get underlying data.
     *
     * @return a pointer to const uint8_t allowing direct access to frame data.
     */
    const uint8_t *getData()
    {
        return reinterpret_cast < const uint8_t * > (&data);
    }

private:

    /**
     * Data structure corresponding to a full M17 data frame.
     */
    typedef struct
    {
        uint16_t   frameNum;  ///< Frame number
        payload_t  payload;   ///< Payload data
    }
    __attribute__((packed)) dataFrame_t;

    dataFrame_t data;         ///< Underlying frame data.
};

#endif /* M17_FRAME_H */
