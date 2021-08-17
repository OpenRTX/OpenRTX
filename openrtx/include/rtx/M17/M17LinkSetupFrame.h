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

#ifndef M17LINKSETUPFRAME_H
#define M17LINKSETUPFRAME_H

#ifndef __cplusplus
#error This header is C++ only!
#endif

#include <string>
#include "M17Datatypes.h"
#include "M17callsign.h"

/**
 * This class describes and handles an M17 Link Setup Frame.
 */
class M17LinkSetupFrame
{
public:

    /**
     * Constructor.
     */
    M17LinkSetupFrame()
    {
        clear();
    }

    /**
     * Destructor.
     */
    ~M17LinkSetupFrame(){ }

    /**
     * Clear the frame content, filling it with zeroes.
     */
    void clear()
    {
        auto *ptr = reinterpret_cast< uint8_t *>(&data);
        std::fill(ptr, ptr + sizeof(lsf_t), 0x00);
    }

    /**
     * Set source callsign.
     *
     * @param callsign: string containing the source callsign.
     */
    void setSource(const std::string& callsign)
    {
        encode_callsign(callsign, data.src);
    }

    /**
     * Set destination callsign.
     *
     * @param callsign: string containing the destination callsign.
     */
    void setDestination(const std::string& callsign)
    {
        encode_callsign(callsign, data.dst);
    }

    /**
     * Get stream type field.
     *
     * @return a copy of the frame's tream type field.
     */
    streamType_t getType()
    {
        // NOTE: M17 fields are big-endian, we need to swap bytes
        uint16_t *a = reinterpret_cast< uint16_t* >(&data.type);
        uint16_t  b = __builtin_bswap16(*a);
        return *reinterpret_cast< streamType_t* >(&b);
    }

    /**
     * Set stream type field.
     *
     * @param type: stream type field to be written.
     */
    void setType(streamType_t type)
    {
        // NOTE: M17 fields are big-endian, we need to swap bytes
        uint16_t *a = reinterpret_cast< uint16_t* >(&type);
        uint16_t  b = __builtin_bswap16(*a);
        data.type   = *reinterpret_cast< streamType_t* >(&b);
    }

    /**
     * Get metadata field.
     *
     * @return a reference to frame's metadata field, allowing for both read and
     * write access.
     */
    meta_t& metadata()
    {
        return data.meta;
    }

    /**
     * Compute a new CRC over the frame content and update the corresponding
     * field.
     */
    void updateCrc();

    /**
     * Get underlying data structure.
     *
     * @return a reference to the underlying dataFrame_t data structure.
     */
    lsf_t& getData()
    {
        return data;
    }

    /**
     * Generate one of the six possible LSF chunks for embedding in data frame's
     * LICH field.
     *
     * @param segmentNum: segment number.
     * @return LSF chunk.
     */
    lsfChunk_t getSegment(const uint8_t segmentNum)
    {
        lsfChunk_t chunk;

        chunk.cnt = segmentNum % 6;
        auto *ptr = reinterpret_cast< uint8_t *>(&data);

        // Copy five bytes from LSF block to the corresponding segment
        for(uint8_t i = 0; i < 5; i++)
        {
            chunk.data[i] = ptr[chunk.cnt + i];
        }

        return chunk;
    }

private:

    lsf_t data;    ///< Underlying frame data.
};

#endif /* M17LINKSETUPFRAME_H */
