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

#ifndef M17FRAMEDECODER_H
#define M17FRAMEDECODER_H

#ifndef __cplusplus
#error This header is C++ only!
#endif

#include <string>
#include <array>
#include "M17LinkSetupFrame.h"
#include "M17Viterbi.h"
#include "M17StreamFrame.h"

enum class M17FrameType : uint8_t
{
    PREAMBLE   = 0,    ///< Frame contains a preamble.
    LINK_SETUP = 1,    ///< Frame is a Link Setup Frame.
    STREAM     = 2,    ///< Frame is a stream data frame.
    PACKET     = 3,    ///< Frame is a packet data frame.
    UNKNOWN    = 4     ///< Frame is unknown.
};

/**
 * M17 frame decoder.
 */
class M17FrameDecoder
{
public:

    /**
     * Constructor.
     */
    M17FrameDecoder();

    /**
     * Destructor.
     */
    ~M17FrameDecoder();

    /**
     * Clear the internal data structures.
     */
    void reset();

    /**
     * Decode an M17 frame, identifying its type. Frame data must contain the
     * sync word in the first two bytes.
     *
     * @param frame: byte array containg frame data.
     * @return the type of frame recognized.
     */
    M17FrameType decodeFrame(const frame_t& frame);

    /**
     * Get the latest Link Setup Frame decoded. Check of the validity of the
     * data contained in the LSF is left to application code.
     *
     * @return a reference to the latest Link Setup Frame decoded.
     */
    const M17LinkSetupFrame& getLsf()
    {
        return lsf;
    }

    /**
     * Get the latest stream data frame decoded.
     *
     * @return a reference to the latest stream data frame decoded.
     */
    const M17StreamFrame& getStreamFrame()
    {
        return streamFrame;
    }

private:

    /**
     * Decode Link Setup Frame data and update the internal LSF field with
     * the new frame data.
     *
     * @param data: byte array containg frame data, without sync word.
     */
    void decodeLSF(const std::array< uint8_t, 46 >& data);

    /**
     * Decode stream data and update the internal LSF field with the new
     * frame data.
     *
     * @param data: byte array containg frame data, without sync word.
     */
    void decodeStream(const std::array< uint8_t, 46 >& data);

    /**
     * Decode a LICH block.
     *
     * @param segment: byte array where to store the decoded Link Setup Frame
     * segment. The last byte contains the segment number.
     * @param lich: LICH block to be decoded.
     * @return true when the LICH block is successfully decoded.
     */
    bool decodeLich(std::array< uint8_t, 6 >& segment, const lich_t& lich);


    uint8_t           lsfSegmentMap;    ///< Bitmap for LSF reassembly from LICH
    M17LinkSetupFrame lsf;              ///< Latest LSF received.
    M17LinkSetupFrame lsfFromLich;      ///< LSF assembled from LICH segments.
    M17StreamFrame    streamFrame;      ///< Latest stream dat frame received.
    M17Viterbi        viterbi;          ///< Viterbi decoder.
};

#endif /* M17FRAMEDECODER_H */
