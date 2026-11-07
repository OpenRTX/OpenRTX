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
     * Decode an M17 frame from its soft bits, one per coded bit, identifying
     * its type from the sync word in the first 16 bits: 0x0000 is a confident
     * 0, 0xFFFF a confident 1 (see Demodulator::getSoftFrame()).
     *
     * @param soft: soft bits of the frame.
     * @return the type of frame recognized.
     */
    FrameType decodeFrame(const softFrame_t &soft);

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
     * Read a payload bit of a frame in coded order, deinterleaving and
     * decorrelating it, so the payload is never copied.
     *
     * @param soft: soft bits of the frame.
     * @param pos: position in the payload, after deinterleaving.
     * @return the soft bit, 0x0000 a confident 0 and 0xFFFF a confident 1.
     */
    static uint16_t payloadBit(const softFrame_t &soft, const size_t pos);

    /**
     * Decode Link Setup Frame data and update the internal LSF field with
     * the new frame data.
     *
     * @param soft: soft bits of the frame.
     */
    void decodeLSF(const softFrame_t &soft);

    /**
     * Decode stream data and update the internal LSF field with the new
     * frame data.
     *
     * @param soft: soft bits of the frame.
     */
    void decodeStream(const softFrame_t &soft);

    /**
     * Decode packet data and update the internal packet frame field with the
     * new frame data.
     *
     * @param soft: soft bits of the frame.
     */
    void decodePacket(const softFrame_t &soft);

    /**
     * Decode a LICH block.
     *
     * @param segment: byte array where to store the decoded Link Setup Frame
     * segment. The last byte contains the segment number.
     * @param soft: soft bits of the stream frame, whose payload starts with
     * the LICH block.
     * @return true when the LICH block is successfully decoded.
     */
    bool decodeLich(std::array<uint8_t, 6> &segment, const softFrame_t &soft);

    ///< Maximum allowed hamming distance when determining the frame type.
    static constexpr uint8_t MAX_SYNC_HAMM_DISTANCE = 4;

    /**
     * A stream payload whose decode differs from the received bits in this
     * many coded bits or more is not copied, keeping the previous frame.
     * Stream frames carry no CRC, so this is the only check between the
     * decoder and codec2.
     */
    static constexpr uint16_t MAX_STREAM_BIT_ERRORS = 20;

    uint8_t lsfSegmentMap;      ///< Bitmap for LSF reassembly from LICH
    LinkSetupFrame lsf;         ///< Latest LSF received.
    LinkSetupFrame lsfFromLich; ///< LSF assembled from LICH segments.
    StreamFrame streamFrame;    ///< Latest stream dat frame received.
    PacketFrame packetFrame;    ///< Latest packet data frame received.
    SoftViterbi viterbi;        ///< Soft-decision Viterbi decoder.
};

} // namespace M17

#endif // FRAMEDECODER_H
