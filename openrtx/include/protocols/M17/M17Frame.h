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
 *   As a special exception, if other files instantiate templates or use   *
 *   macros or inline functions from this file, or you compile this file   *
 *   and link it with other works to produce a work based on this file,    *
 *   this file does not by itself cause the resulting work to be covered   *
 *   by the GNU General Public License. However the source code for this   *
 *   file must still be made available in accordance with the GNU General  *
 *   Public License. This exception does not invalidate any other reasons  *
 *   why a work based on this file might be covered by the GNU General     *
 *   Public License.                                                       *
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
     * Get underlying data structure.
     *
     * @return a reference to the underlying dataFrame_t data structure.
     */
    dataFrame_t& getData()
    {
        return data;
    }

    /**
     * Dump the frame content to a std::array.
     *
     * \return std::array containing the content of the frame.
     */
    std::array< uint8_t, sizeof(dataFrame_t) > toArray()
    {
        std::array< uint8_t, sizeof(dataFrame_t) > frame;
        memcpy(frame.data(), &data, frame.size());
        return frame;
    }

private:

    dataFrame_t data;   ///< Underlying frame data.
};

#endif /* M17_FRAME_H */
