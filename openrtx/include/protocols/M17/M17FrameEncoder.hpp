/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef M17FRAMEENCODER_H
#define M17FRAMEENCODER_H

#ifndef __cplusplus
#error This header is C++ only!
#endif

#include <string>
#include <array>
#include "M17ConvolutionalEncoder.hpp"
#include "M17LinkSetupFrame.hpp"
#include "M17StreamFrame.hpp"

namespace M17
{

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

    /**
     * Encode an End Of Transmission marker frame.
     *
     * @param output: destination buffer for the encoded data.
     */
    void encodeEotFrame(frame_t& output);

    /**
     * Get a copy of the LSF data belonging to the current transmission.
     *
     * @return LSF data.
     */
    M17LinkSetupFrame getCurrentLsf()
    {
        return currLsf;
    }

    /**
     * Update the Link Setup Frame data for the current transmission.
     * The new LSF data will become active once all the LICH segments of the
     * previous LSF have been sent.
     *
     * @param lsf: new Link Setup Frame to be sent.
     */
    void updateLsfData(M17LinkSetupFrame& lsf)
    {
        newLsf = lsf;
        newLsf.updateCrc();
        updateLsf = true;
    }

private:

    M17ConvolutionalEncoder  encoder;           ///< Convolutional encoder.
    M17LinkSetupFrame        currLsf;           ///< LSF of current transmission.
    M17LinkSetupFrame        newLsf;            ///< Next LSF to be sent.
    uint8_t                  currentLich;       ///< Index of current LSF chunk.
    uint16_t                 streamFrameNumber; ///< Current frame number.
    bool                     updateLsf;         ///< LSF data needs update.
};

}      // namespace M17

#endif // M17FRAMEENCODER_H
