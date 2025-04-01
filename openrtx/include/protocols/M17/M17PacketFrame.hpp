/***************************************************************************
 *   Copyright (C) 2021 - 2023 by Federico Amedeo Izzo IU2NUO,             *
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

#ifndef M17_PACKET_FRAME_H
#define M17_PACKET_FRAME_H

#ifndef __cplusplus
#error This header is C++ only!
#endif

#include <cstring>
#include <string>
#include "M17Datatypes.hpp"

namespace M17
{

class M17FrameDecoder;

/**
 * This class describes and handles a generic M17 packet frame.
 */
class M17PacketFrame
{
public:

    /**
     * Constructor.
     */
    M17PacketFrame()
    {
        clear();
    }

    /**
     * Destructor.
     */
    ~M17PacketFrame(){ }

    /**
     * Clear the frame content, filling it with zeroes.
     */
    void clear()
    {
        memset(&data, 0x00, sizeof(data));
    }

    /**
     * Access frame payload.
     *
     * @return a reference to frame's payload field, allowing for both read and
     * write access.
     */
    pktPayload_t& payload()
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

    struct __attribute__((packed))
    {
        pktPayload_t payload;   // Payload data
    }
    data;
                                                   ///< Frame data.
    // Frame decoder class needs to access raw frame data
    friend class M17FrameDecoder;
};

}      // namespace M17

#endif // M17_PACKET_FRAME_H
