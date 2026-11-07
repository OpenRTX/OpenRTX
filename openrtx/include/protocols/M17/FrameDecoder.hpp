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
     * sync word in the first two bytes. The payload is decoded with a
     * soft-decision Viterbi using the per-bit confidence values in @p soft,
     * which are parallel to the coded bits of @p frame: 0x0000 is a confident
     * 0, 0xFFFF a confident 1 (see Demodulator::getSoftFrame()).
     *
     * @param frame: byte array containing frame data.
     * @param soft: soft bits, one per coded bit of the frame.
     * @return the type of frame recognized.
     */
    FrameType decodeFrame(const frame_t &frame, const softFrame_t &soft);

    /**
     * Decode an M17 frame from hard bits only, every bit read as a fully
     * confident soft bit.
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
     * Get the number of coded bits corrected in the latest stream payload
     * decode, see MAX_STREAM_BIT_ERRORS.
     *
     * @return corrected bit count of the latest stream payload decode.
     */
    uint16_t getStreamBitErrors() const
    {
        return streamBitErrors;
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

    /**
     * A stream payload whose decode corrected this many coded bits or more is
     * not copied, keeping the previous frame. Stream frames carry no CRC, so
     * this is the only check between the decoder and codec2.
     */
    static constexpr uint16_t MAX_STREAM_BIT_ERRORS = 20;

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
     * Soft bits of a frame payload, read in coded order.
     */
    class PayloadBits;

    /**
     * Decode an M17 frame from its soft bits, or from its hard bits as fully
     * confident ones.
     *
     * @param frame: byte array containing frame data.
     * @param soft: soft bits of the frame, or nullptr to use the hard bits.
     * @return the type of frame recognized.
     */
    FrameType decode(const frame_t &frame, const softFrame_t *soft);

    /**
     * Decode Link Setup Frame data and update the internal LSF field with
     * the new frame data.
     *
     * @param payload: soft bits of the frame payload.
     */
    void decodeLSF(const PayloadBits &payload);

    /**
     * Decode stream data and update the internal LSF field with the new
     * frame data.
     *
     * @param payload: soft bits of the frame payload.
     */
    void decodeStream(const PayloadBits &payload);

    /**
     * Decode packet data and update the internal packet frame field with the
     * new frame data.
     *
     * @param payload: soft bits of the frame payload.
     */
    void decodePacket(const PayloadBits &payload);

    /**
     * Decode a LICH block.
     *
     * @param segment: byte array where to store the decoded Link Setup Frame
     * segment. The last byte contains the segment number.
     * @param payload: soft bits of the stream frame payload, which starts with
     * the LICH block.
     * @return true when the LICH block is successfully decoded.
     */
    bool decodeLich(std::array<uint8_t, 6> &segment,
                    const PayloadBits &payload);

    uint8_t lsfSegmentMap;        ///< Bitmap for LSF reassembly from LICH
    LinkSetupFrame lsf;           ///< Latest LSF received.
    LinkSetupFrame lsfFromLich;   ///< LSF assembled from LICH segments.
    StreamFrame streamFrame;      ///< Latest stream dat frame received.
    PacketFrame packetFrame;      ///< Latest packet data frame received.
    SoftViterbi viterbi;          ///< Soft-decision Viterbi decoder.
    uint16_t streamBitErrors = 0; ///< Corrected bits in the latest stream.

    ///< Maximum allowed hamming distance when determining the frame type.
    static constexpr uint8_t MAX_SYNC_HAMM_DISTANCE = 4;
};

} // namespace M17

#endif // FRAMEDECODER_H
