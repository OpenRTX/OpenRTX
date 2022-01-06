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

#ifndef M17FRAMEENCODER_H
#define M17FRAMEENCODER_H

#ifndef __cplusplus
#error This header is C++ only!
#endif

#include <string>
#include <array>
#include "M17ConvolutionalEncoder.h"
#include "M17LinkSetupFrame.h"
#include "M17StreamFrame.h"

/**
 * M17 frame encoder.
 */
class M17FrameEncoder
{
public:

    /**
     * Constructor.
     */
    M17FrameEncoder();

    /**
     * Destructor.
     */
    ~M17FrameEncoder();

    /**
     * Clear the internal data structures, reset the counter for frame sequence
     * number in stream data frames and reset the counter for LICH segment
     * sequence.
     */
    void reset();

    /**
     * Encode a Link Setup Frame into a frame ready for transmission, prepended
     * with the corresponding sync word. Link Setup data is also copied to an
     * internal data structure and used to generate the LICH segments to be
     * placed in each stream frame.
     *
     * @param lsf: Link Setup Frame to be encoded.
     * @param output: destination buffer for the encoded data.
     */
    void encodeLsf(M17LinkSetupFrame& lsf, frame_t& output);

    /**
     * Prepare and encode a stream data frame into a frame ready for
     * transmission, prepended with the corresponding sync word. The frame
     * sequence number is incremented by one on each function call and cleared
     * when the reset() function is called. The LICH segment field is filled
     * with data obtained from the latest Link Setup Frame encoded.
     *
     * @param payload: payload data.
     * @param output: destination buffer for the encoded data.
     * @param isLast: if true, current frame is marked as the last one to be
     * transmitted.
     * @return the frame sequence number.
     */
    uint16_t encodeStreamFrame(const payload_t& payload, frame_t& output,
                               const bool isLast = false);
private:

    M17ConvolutionalEncoder  encoder;           ///< Convolutional encoder.
    std::array< lich_t, 6 >  lichSegments;      ///< Encoded LSF chunks for LICH generation.
    uint8_t                  currentLich;       ///< Index of current LSF chunk.
    uint16_t                 streamFrameNumber; ///< Current frame number.
};

#endif /* M17FRAMEENCODER_H */
