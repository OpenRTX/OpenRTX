/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef M17_LINKSETUPFRAME_H
#define M17_LINKSETUPFRAME_H

#ifndef __cplusplus
#error This header is C++ only!
#endif

#include <string>
#include <array>
#include "protocols/M17/M17Datatypes.hpp"
#include "protocols/M17/Callsign.hpp"
#include "core/gps.h"

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
    void setSource(const Callsign& callsign);

    /**
     * Get source callsign.
     *
     * @return: string containing the source callsign.
     */
    Callsign getSource();

    /**
     * Set destination callsign.
     *
     * @param callsign: string containing the destination callsign.
     */
    void setDestination(const Callsign& callsign);

    /**
     * Get destination callsign.
     *
     * @return: string containing the destination callsign.
     */
    Callsign getDestination();

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
     * @param segment: LSF chunk to be filled
     * @param segmentNum: segment number, between 0 and 5.
     */
    void generateLichSegment(lich_t &segment, const uint8_t segmentNum);

    /**
     * @brief Set the GNSS data for the LSF's meta feature. This method not only
     * handles converting the datatypes, but it also handles the byte swapping
     * necessary since M17 is big endian but the runtime is little endian.
     *
     * @param position
     * @param stationType
     */
    void setGnssData(const gps_t *position, const M17GNSSStationType stationType);

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
