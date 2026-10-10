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
#include <memory>
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
     * Allocate the soft-decision scratch buffers. Call before decodeFrame().
     */
    void init();

    /**
     * Release the soft-decision scratch buffers allocated by init(). Call when
     * the decoder is not going to be used for a while.
     */
    void terminate();

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
     * Decode an M17 frame from hard bits only. Every bit is treated as fully
     * confident, which makes the decoder behave as a hard-decision one.
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
    static constexpr uint16_t MAX_STREAM_BIT_ERRORS = 17;

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
     * @param soft: soft bits of the frame payload, decorrelated and
     * deinterleaved, without sync word.
     */
    void decodeLSF(const std::array<uint16_t, 368> &soft);

    /**
     * Decode stream data and update the internal LSF field with the new
     * frame data.
     *
     * @param soft: soft bits of the frame payload, decorrelated and
     * deinterleaved, without sync word.
     */
    void decodeStream(const std::array<uint16_t, 368> &soft);

    /**
     * Decode packet data and update the internal packet frame field with the
     * new frame data.
     *
     * @param soft: soft bits of the frame payload, decorrelated and
     * deinterleaved, without sync word.
     */
    void decodePacket(const std::array<uint16_t, 368> &soft);

    /**
     * Decode a LICH block.
     *
     * @param segment: byte array where to store the decoded Link Setup Frame
     * segment. The last byte contains the segment number.
     * @param lich: LICH block to be decoded.
     * @return true when the LICH block is successfully decoded.
     */
    bool decodeLich(std::array<uint8_t, 6> &segment, const lich_t &lich);

    /**
     * Soft-decision scratch buffers. They are too large for the stack of the
     * RTX thread, where frames are decoded, so init() allocates them on the
     * heap and terminate() releases them while the decoder is not in use.
     */
    struct SoftScratch {
        std::array<uint16_t, 368> payload; ///< Decorrelated, deinterleaved.
        std::array<uint16_t, 272> stream;  ///< Stream payload after the LICH.
    };

    uint8_t lsfSegmentMap;      ///< Bitmap for LSF reassembly from LICH
    LinkSetupFrame lsf;         ///< Latest LSF received.
    LinkSetupFrame lsfFromLich; ///< LSF assembled from LICH segments.
    StreamFrame streamFrame;    ///< Latest stream dat frame received.
    PacketFrame packetFrame;    ///< Latest packet data frame received.
    SoftViterbi viterbi;        ///< Soft-decision Viterbi decoder.
    std::unique_ptr<SoftScratch> scratch; ///< Soft-decision scratch buffers.
    uint16_t streamBitErrors = 0; ///< Corrected bits in the latest stream.

    ///< Maximum allowed hamming distance when determining the frame type.
    static constexpr uint8_t MAX_SYNC_HAMM_DISTANCE = 4;
};

} // namespace M17

#endif // FRAMEDECODER_H
