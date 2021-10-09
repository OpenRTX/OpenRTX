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

#ifndef M17_LINKSETUPFRAME_H
#define M17_LINKSETUPFRAME_H

#ifndef __cplusplus
#error This header is C++ only!
#endif

#include <string>
#include <array>
#include "M17Datatypes.h"

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
     * Set destination callsign.
     *
     * @param callsign: string containing the destination callsign.
     */
    void setDestination(const std::string& callsign);

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
     * Get underlying data structure.
     *
     * @return a reference to the underlying dataFrame_t data structure.
     */
    lsf_t& getData();

    /**
     * Generate one of the six possible LSF chunks for embedding in data frame's
     * LICH field. Output is the Golay (24,12) encoded LSF chunk.
     *
     * @param segmentNum: segment number, between 0 and 5.
     * @return Golay (24,12) encoded LSF chunk.
     */
    lich_t generateLichSegment(const uint8_t segmentNum);

    /**
     * Dump the frame content to a std::array.
     *
     * \return std::array containing the content of the frame.
     */
    std::array< uint8_t, sizeof(lsf_t) > toArray();

private:

    /**
     * Compute the CRC16 of a given chunk of data using the polynomial 0x5935
     * with an initial value set to 0xFFFF, as per M17 specification.
     *
     * \param data: pointer to the data block.
     * \param len: lenght of the data block, in bytes.
     * \return computed CRC16 over the data block.
     */
    uint16_t crc16(const void *data, const size_t len);

    lsf_t data;    ///< Underlying frame data.
};

#endif /* M17_LINKSETUPFRAME_H */
