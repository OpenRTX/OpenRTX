/***************************************************************************
 *   Copyright (C) 2021 - 2022 by Federico Amedeo Izzo IU2NUO,             *
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

#ifndef M17_LINKSETUPFRAME_H
#define M17_LINKSETUPFRAME_H

#ifndef __cplusplus
#error This header is C++ only!
#endif

#include <string>
#include <array>
#include "M17Datatypes.hpp"

namespace M17
{

class M17FrameDecoder;

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
    M17LinkSetupFrame();

    /**
     * Destructor.
     */
    ~M17LinkSetupFrame();

    /**
     * Clear the frame content, filling it with zeroes and resetting the
     * destination address to broadcast.
     */
    void clear();

    /**
     * Set source callsign.
     *
     * @param callsign: string containing the source callsign.
     */
    void setSource(const std::string& callsign);

    /**
     * Get source callsign.
     *
     * @return: string containing the source callsign.
     */
    std::string getSource();

    /**
     * Set destination callsign.
     *
     * @param callsign: string containing the destination callsign.
     */
    void setDestination(const std::string& callsign);

    /**
     * Get destination callsign.
     *
     * @return: string containing the destination callsign.
     */
    std::string getDestination();

    /**
     * Get stream type field.
     *
     * @return a copy of the frame's tream type field.
     */
    streamType_t getType();

    /**
     * Set stream type field.
     *
     * @param type: stream type field to be written.
     */
    void setType(streamType_t type);

    /**
     * Get metadata field.
     *
     * @return a reference to frame's metadata field, allowing for both read and
     * write access.
     */
    meta_t& metadata();

    /**
     * Compute a new CRC over the frame content and update the corresponding
     * field.
     */
    void updateCrc();

    /**
     * Check if frame data is valid that is, if the CRC computed over the LSF
     * fields matches the one carried by the LSF itself.
     *
     * @return true if CRC of LSF data matches the one stored in the LSF itself,
     * false otherwise.
     */
    bool valid() const;

    /**
     * Get underlying data.
     *
     * @return a pointer to const uint8_t allowing direct access to LSF data.
     */
    const uint8_t *getData();

    /**
     * Generate one of the six possible LSF chunks for embedding in data frame's
     * LICH field. Output is the Golay (24,12) encoded LSF chunk.
     *
     * @param segmentNum: segment number, between 0 and 5.
     * @return Golay (24,12) encoded LSF chunk.
     */
    lich_t generateLichSegment(const uint8_t segmentNum);

private:

    /**
     * Compute the CRC16 of a given chunk of data using the polynomial 0x5935
     * with an initial value set to 0xFFFF, as per M17 specification.
     *
     * \param data: pointer to the data block.
     * \param len: lenght of the data block, in bytes.
     * \return computed CRC16 over the data block.
     */
    uint16_t crc16(const void *data, const size_t len) const;


    struct __attribute__((packed))
    {
        call_t       dst;    ///< Destination callsign
        call_t       src;    ///< Source callsign
        streamType_t type;   ///< Stream type information
        meta_t       meta;   ///< Metadata
        uint16_t     crc;    ///< CRC
    }
    data;                    ///< Frame data.

    // Frame decoder class needs to access raw frame data
    friend class M17FrameDecoder;
};

}      // namespace M17

#endif // M17_LINKSETUPFRAME_H
