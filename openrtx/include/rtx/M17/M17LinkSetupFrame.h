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

#include <cstring>
#include <string>
#include <array>
#include "M17Datatypes.h"
#include "M17Callsign.h"
#include "M17Golay.h"

/**
 * This class describes and handles an M17 Link Setup Frame.
 * By default the frame contains a broadcast destination address, unless a
 * specific address is set by calling the corresponding function.
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
     * Clear the frame content, filling it with zeroes and resetting the
     * destination address to broadcast.
     */
    void clear()
    {
        memset(&data, 0x00, sizeof(lsf_t));
        data.dst.fill(0xFF);
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
    void updateCrc()
    {
        // Compute CRC over the first 28 bytes, then store it in big endian format.
        uint16_t crc = crc16(&data, 28);
        data.crc     = __builtin_bswap16(crc);
    }

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
     * LICH field. Output is the Golay (24,12) encoded LSF chunk.
     *
     * @param segmentNum: segment number, between 0 and 5.
     * @return Golay (24,12) encoded LSF chunk.
     */
    lich_t generateLichSegment(const uint8_t segmentNum)
    {
        /*
         * The M17 protocol specification prescribes that the content of the
         * link setup frame is continuously transmitted alongside data frames
         * by partitioning it in 6 chunks of five bites each and cyclically
         * transmitting these chunks.
         * With a bit of pointer math, we extract the data for each of the
         * chunks by casting the a pointer lsf_t data structure to uint8_t
         * and adjusting it to make it point to the start of each block of
         * five bytes, as specified by the segmentNum parameter.
         *
         */

        // Set up pointer to the beginning of the specified 5-byte chunk
        uint8_t num    = segmentNum % 6;
        uint8_t *chunk = reinterpret_cast< uint8_t* >(&data) + (num * 5);

        // Partition chunk data in 12-bit blocks for Golay(24,12) encoding.
        std::array< uint16_t, 4 > blocks;
        blocks[0] = chunk[0] << 4 | ((chunk[1] >> 4) & 0x0F);
        blocks[1] = ((chunk[1] & 0x0F) << 8) | chunk[2];
        blocks[2] = chunk[3] << 4 | ((chunk[4] >> 4) & 0x0F);
        blocks[3] = ((chunk[4] & 0x0F) << 8) | (num << 5);

        // Encode each block and assemble the final data block.
        // NOTE: shift and bitswap required to genereate big-endian data.
        lich_t result;
        for(size_t i = 0; i < blocks.size(); i++)
        {
            uint32_t encoded = golay_encode24(blocks[i]);
            encoded          = __builtin_bswap32(encoded << 8);
            memcpy(&result[3*i], &encoded, 3);
        }

        return result;
    }

    /**
     * Dump the frame content to a std::array.
     *
     * \return std::array containing the content of the frame.
     */
    std::array< uint8_t, sizeof(lsf_t) > toArray()
    {
        std::array< uint8_t, sizeof(lsf_t) > frame;
        memcpy(frame.data(), &data, frame.size());
        return frame;
    }

private:

    /**
     * Compute the CRC16 of a given chunk of data using the polynomial 0x5935
     * with an initial value set to 0xFFFF, as per M17 specification.
     *
     * \param data: pointer to the data block.
     * \param len: lenght of the data block, in bytes.
     * \return computed CRC16 over the data block.
     */
    uint16_t crc16(const void *data, const size_t len)
    {
        const uint8_t *ptr = reinterpret_cast< const uint8_t *>(data);
        uint16_t crc = 0xFFFF;

        for(size_t i = 0; i < len; i++)
        {
            crc ^= (ptr[i] << 8);

            for(uint8_t j = 0; j < 8; j++)
            {
                if(crc & 0x8000)
                    crc = (crc << 1) ^ 0x5935;
                else
                    crc = (crc << 1);
            }
        }

        return crc;
    }

    lsf_t data;    ///< Underlying frame data.
};

#endif /* M17LINKSETUPFRAME_H */
