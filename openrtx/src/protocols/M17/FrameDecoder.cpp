/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "protocols/M17/Golay.hpp"
#include "protocols/M17/FrameDecoder.hpp"
#include "protocols/M17/FrameEncoder.hpp"
#include "protocols/M17/Interleaver.hpp"
#include "protocols/M17/Decorrelator.hpp"
#include "protocols/M17/CodePuncturing.hpp"
#include "protocols/M17/Constants.hpp"
#include "protocols/M17/Utils.hpp"
#include <algorithm>

using namespace M17;

static constexpr size_t SYNC_BITS = 2 * SYNCWORD_SYMBOLS;
static constexpr size_t PAYLOAD_BITS = 2 * (FRAME_SYMBOLS - SYNCWORD_SYMBOLS);
static constexpr size_t LICH_BITS = 8 * sizeof(lich_t);
static constexpr size_t STREAM_BITS = PAYLOAD_BITS - LICH_BITS;

/**
 * Whether a frame of this type carries a payload to decode.
 *
 * @param type: frame type, from the sync word.
 * @return true for link setup, stream and packet frames.
 */
static bool hasPayload(FrameType type)
{
    return (type == FrameType::LINK_SETUP) || (type == FrameType::STREAM)
        || (type == FrameType::PACKET);
}

FrameDecoder::FrameDecoder()
{
}

FrameDecoder::~FrameDecoder()
{
}

void FrameDecoder::reset()
{
    lsfSegmentMap = 0;
    lsf.clear();
    lsfFromLich.clear();
    streamFrame.clear();
    packetFrame.clear();
    streamBitErrors = 0;
}

/**
 * Soft bits of a frame payload, read in coded order: each read deinterleaves
 * and decorrelates one bit of the frame, so the payload is never copied.
 */
class FrameDecoder::PayloadBits
{
public:
    /**
     * Constructor.
     *
     * @param frame: frame, starting with the sync word.
     * @param soft: soft bits of the frame, or nullptr to read the hard bits of
     * the frame as fully confident ones.
     */
    PayloadBits(const frame_t &frame, const softFrame_t *soft)
        : frame(frame), soft(soft)
    {
    }

    /**
     * Read a payload bit.
     *
     * @param pos: position in the payload, after deinterleaving.
     * @return the soft bit, 0x0000 a confident 0 and 0xFFFF a confident 1.
     */
    uint16_t operator()(size_t pos) const
    {
        size_t index = interleavedIndex(pos, PAYLOAD_BITS);
        uint16_t value;
        if (soft != nullptr)
            value = (*soft)[SYNC_BITS + index];
        else
            value = getBit(frame, SYNC_BITS + index) ? 0xFFFF : 0x0000;

        // Decorrelating a soft bit inverts its confidence
        if (getBit(sequence, index))
            value = 0xFFFF - value;

        return value;
    }

private:
    const frame_t &frame;    ///< Frame, starting with the sync word.
    const softFrame_t *soft; ///< Soft bits of the frame, or nullptr.
};

FrameType FrameDecoder::decodeFrame(const frame_t &frame)
{
    return decode(frame, nullptr);
}

FrameType FrameDecoder::decodeFrame(const frame_t &frame,
                                    const softFrame_t &soft)
{
    return decode(frame, &soft);
}

FrameType FrameDecoder::decode(const frame_t &frame, const softFrame_t *soft)
{
    std::array<uint8_t, 2> syncWord;
    std::copy_n(frame.begin(), 2, syncWord.begin());
    auto type = getFrameType(syncWord);

    // Preamble, EOT and unknown frames carry no payload to decode.
    if (!hasPayload(type))
        return type;

    PayloadBits payload(frame, soft);

    switch (type) {
        case FrameType::LINK_SETUP:
            decodeLSF(payload);
            break;
        case FrameType::STREAM:
            decodeStream(payload);
            break;
        case FrameType::PACKET:
            decodePacket(payload);
            break;
        default:
            break;
    }

    return type;
}

FrameType FrameDecoder::getFrameType(const std::array<uint8_t, 2> &syncWord)
{
    // Preamble
    FrameType type = FrameType::PREAMBLE;
    uint8_t minDistance = hammingDistance(syncWord[0], 0x77)
                        + hammingDistance(syncWord[1], 0x77);

    // Link setup frame
    uint8_t hammDistance = hammingDistance(syncWord[0], LSF_SYNC_WORD[0])
                         + hammingDistance(syncWord[1], LSF_SYNC_WORD[1]);

    if (hammDistance < minDistance) {
        type = FrameType::LINK_SETUP;
        minDistance = hammDistance;
    }

    // Stream frame
    hammDistance = hammingDistance(syncWord[0], STREAM_SYNC_WORD[0])
                 + hammingDistance(syncWord[1], STREAM_SYNC_WORD[1]);

    if (hammDistance < minDistance) {
        type = FrameType::STREAM;
        minDistance = hammDistance;
    }

    // Packet frame
    hammDistance = hammingDistance(syncWord[0], PACKET_SYNC_WORD[0])
                 + hammingDistance(syncWord[1], PACKET_SYNC_WORD[1]);

    if (hammDistance < minDistance) {
        type = FrameType::PACKET;
        minDistance = hammDistance;
    }

    // EOT frame
    hammDistance = hammingDistance(syncWord[0], EOT_SYNC_WORD[0])
                 + hammingDistance(syncWord[1], EOT_SYNC_WORD[1]);

    if (hammDistance < minDistance) {
        type = FrameType::EOT;
        minDistance = hammDistance;
    }

    // Check value of minimum hamming distance found, if exceeds the allowed
    // limit consider the frame as of unknown type.
    if (minDistance > MAX_SYNC_HAMM_DISTANCE) {
        type = FrameType::UNKNOWN;
    }

    return type;
}

void FrameDecoder::decodeLSF(const PayloadBits &payload)
{
    std::array<uint8_t, sizeof(LinkSetupFrame)> tmp;
    viterbi.decodePunctured<PAYLOAD_BITS>(payload, tmp, LSF_PUNCTURE);
    memcpy(&lsf.data, tmp.data(), tmp.size());
}

void FrameDecoder::decodePacket(const PayloadBits &payload)
{
    // Extract and decode packet data
    std::array<uint8_t, PacketFrame::FRAME_SIZE> tmp;

    viterbi.decodePunctured<PAYLOAD_BITS>(payload, tmp, PACKET_PUNCTURE);

    // Viterbi decoding of P3-punctured packets produces a 2-bit right shift:
    // encoding 26 bytes (208 bits) with flush gives 210 Viterbi steps → 420
    // coded bits, punctured by P3 to 368 bits (46 bytes). The 210-step decode
    // outputs 208 bits, leaving a 2-bit offset. Realign left 2 bits; the last
    // byte's low 2 bits become zero (spec reserved bits).
    for (size_t i = 0; i < tmp.size(); ++i) {
        uint8_t currentByte = tmp[i];
        uint8_t nextByte = (i < tmp.size() - 1) ? tmp[i + 1] : 0;

        tmp[i] = (currentByte << 2) | (nextByte >> 6);
    }

    // No error limit here: the packet CRC checked by the deframer is the
    // authority, and a frame that decoded badly ends the packet either way.
    memcpy(&packetFrame.frameData, tmp.data(), tmp.size());
}

void FrameDecoder::decodeStream(const PayloadBits &payload)
{
    std::array<uint8_t, 6> lsfSegment;
    bool decodeOk = decodeLich(lsfSegment, payload);
    if (decodeOk) {
        // Append LICH segment
        uint8_t segmentNum = lsfSegment[5];
        uint8_t segmentSize = lsfSegment.size() - 1;
        uint8_t *ptr = reinterpret_cast<uint8_t *>(&lsfFromLich.data);
        ptr += segmentNum * segmentSize;
        memcpy(ptr, lsfSegment.data(), segmentSize);

        // Mark this segment as present
        lsfSegmentMap |= 1 << segmentNum;

        // Check if we have received all the six LICH segments
        if (lsfSegmentMap == 0x3F) {
            if (lsfFromLich.valid())
                lsf = lsfFromLich;

            lsfSegmentMap = 0;
            lsfFromLich.clear();
        }
    }

    // Decode the stream data, which follows the LICH
    auto stream = [&payload](size_t pos) { return payload(LICH_BITS + pos); };

    std::array<uint8_t, sizeof(StreamFrame)> tmp;
    viterbi.decodePunctured<STREAM_BITS>(stream, tmp, DATA_PUNCTURE);

    StreamFrame decoded;
    memcpy(&decoded.frameData, tmp.data(), tmp.size());

    // Acceptance check: re-encode the decoded payload as the transmitter did
    // and count the coded bits that differ from the received hard decisions.
    // A decode from noise disagrees with many of them, a genuine frame only
    // where the channel flipped a bit.
    std::array<uint8_t, 34> punctured;
    FrameEncoder::encodeStreamPayload(decoded, punctured);

    uint16_t errors = 0;
    for (size_t i = 0; i < STREAM_BITS; i++) {
        bool received = stream(i) >= 0x8000;
        if (received != getBit(punctured, i))
            errors++;
    }
    streamBitErrors = errors;

    // A rejected payload is not copied, so the previous stream frame is kept.
    if (errors < MAX_STREAM_BIT_ERRORS)
        streamFrame = decoded;
}

bool FrameDecoder::decodeLich(std::array<uint8_t, 6> &segment,
                              const PayloadBits &payload)
{
    /*
     * Extract and unpack the LICH segment contained in the frame header.
     * The LICH segment is composed of four blocks of Golay(24,12) encoded data,
     * the first 96 bits of the payload, and carries five bytes of the original
     * Link Setup Frame. The sixth byte is the segment number, allowing to
     * determine the correct position of the segment when reassembling the LSF.
     */

    segment.fill(0x00);

    size_t index = 0;
    std::array<uint16_t, 24> block;

    for (size_t i = 0; i < 4; i++) {
        for (size_t j = 0; j < block.size(); j++)
            block[j] = payload(block.size() * i + j);

        uint16_t decoded = golay24_softDecode(block);

        // Unrecoverable error, abort decoding
        if (decoded == 0xFFFF) {
            segment.fill(0x00);
            return false;
        }

        if (i & 1) {
            segment[index++] |= (decoded >> 8);     // upper 4 bits
            segment[index++] = (decoded & 0xFF);    // lower 8 bits
        } else {
            segment[index++] |= (decoded >> 4);     // upper 8 bits
            segment[index] = (decoded & 0x0F) << 4; // lower 4 bits
        }
    }

    // Last byte of the segment contains the segment number, shift left
    // by five when packing the LICH. The segment number must range between
    // zero and five.
    segment[5] >>= 5;

    if (segment[5] > 5)
        return false;

    return true;
}
