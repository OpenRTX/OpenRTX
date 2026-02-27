/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef FRAMEDECODER_H
#define FRAMEDECODER_H

#ifndef __cplusplus
#error This header is C++ only!
#endif

#include <cstdint>
#include <string>
#include <array>
#include "LinkSetupFrame.hpp"
#include "Viterbi.hpp"
#include "StreamFrame.hpp"
#include "PacketFrame.hpp"

namespace M17
{

enum class FrameType : uint8_t {
    PREAMBLE = 0,   ///< Frame contains a preamble.
    LINK_SETUP = 1, ///< Frame is a Link Setup Frame.
    STREAM = 2,     ///< Frame is a stream data frame.
    PACKET = 3,     ///< Frame is a packet data frame.
    EOT = 4,        ///< Frame is an End Of Transmission frame.
    UNKNOWN = 5     ///< Frame is unknown.
};

/**
 * M17 frame decoder.
 */
class FrameDecoder
{
public:
    /**
     * Constructor.
     */
    FrameDecoder();

    /**
     * Destructor.
     */
    ~FrameDecoder();

    /**
     * Clear the internal data structures.
     */
    void reset();

    /**
     * Decode an M17 frame, identifying its type. Frame data must contain the
     * sync word in the first two bytes.
     *
     * @param frame: byte array containing frame data.
     * @return the type of frame recognized.
     */
    FrameType decodeFrame(const frame_t &frame);

    /**
     * Get the latest Link Setup Frame decoded. Check of the validity of the
     * data contained in the LSF is left to application code.
     *
     * @return a reference to the latest Link Setup Frame decoded.
     */
    const LinkSetupFrame &getLsf()
    {
        return lsf;
    }

    /**
     * Get the latest stream data frame decoded.
     *
     * @return a reference to the latest stream data frame decoded.
     */
    const StreamFrame &getStreamFrame()
    {
        return streamFrame;
    }

    /**
     * Get the latest packet data frame decoded.
     *
     * @return a reference to the latest packet data frame decoded.
     */
    const PacketFrame &getPacketFrame() const
    {
        return packetFrame;
    }

private:
    /**
     * Determine frame type by searching which syncword among the standard M17
     * ones has the minumum hamming distance from the given one. If the hamming
     * distance exceeds a masimum absolute threshold the frame is declared of
     * unknown type.
     *
     * @param syncWord: frame syncword.
     * @return frame type based on the given syncword.
     */
    FrameType getFrameType(const std::array<uint8_t, 2> &syncWord);

    /**
     * Decode Link Setup Frame data and update the internal LSF field with
     * the new frame data.
     *
     * @param data: byte array containing frame data, without sync word.
     */
    void decodeLSF(const std::array<uint8_t, 46> &data);

    /**
     * Decode stream data and update the internal LSF field with the new
     * frame data.
     *
     * @param data: byte array containing frame data, without sync word.
     */
    void decodeStream(const std::array<uint8_t, 46> &data);

    /**
     * Decode packet data and update the internal packet frame field with the
     * new frame data.
     *
     * @param data: byte array containing frame data, without sync word.
     */
    void decodePacket(const std::array<uint8_t, 46> &data);

    /**
     * Decode a LICH block.
     *
     * @param segment: byte array where to store the decoded Link Setup Frame
     * segment. The last byte contains the segment number.
     * @param lich: LICH block to be decoded.
     * @return true when the LICH block is successfully decoded.
     */
    bool decodeLich(std::array<uint8_t, 6> &segment, const lich_t &lich);

    uint8_t lsfSegmentMap;      ///< Bitmap for LSF reassembly from LICH
    LinkSetupFrame lsf;         ///< Latest LSF received.
    LinkSetupFrame lsfFromLich; ///< LSF assembled from LICH segments.
    StreamFrame streamFrame;    ///< Latest stream dat frame received.
    PacketFrame packetFrame;    ///< Latest packet data frame received.
    HardViterbi viterbi;        ///< Viterbi decoder.

    ///< Maximum allowed hamming distance when determining the frame type.
    static constexpr uint8_t MAX_SYNC_HAMM_DISTANCE = 4;

    ///< Maximum number of corrected bit errors allowed in a stream frame.
    static constexpr uint16_t MAX_VITERBI_ERRORS = 15;
};

} // namespace M17

#endif // FRAMEDECODER_H
