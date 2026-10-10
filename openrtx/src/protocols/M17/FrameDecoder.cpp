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

void FrameDecoder::init()
{
    scratch = std::make_unique<SoftScratch>();
}

void FrameDecoder::terminate()
{
    scratch.reset();
}

FrameType FrameDecoder::decodeFrame(const frame_t &frame)
{
    // Hard-input convenience for tests and loopbacks; not used on the RTX
    // thread, so the 768-byte scratch buffer can live on the stack.
    softFrame_t soft;
    for (size_t i = 0; i < soft.size(); i++)
        soft[i] = getBit(frame, i) ? 0xFFFF : 0x0000;

    return decodeFrame(frame, soft);
}

FrameType FrameDecoder::decodeFrame(const frame_t &frame,
                                    const softFrame_t &soft)
{
    std::array<uint8_t, 2> syncWord;
    std::copy_n(frame.begin(), 2, syncWord.begin());
    auto type = getFrameType(syncWord);

    // Preamble, EOT and unknown frames carry no payload to decode.
    if ((type != FrameType::LINK_SETUP) && (type != FrameType::STREAM)
        && (type != FrameType::PACKET))
        return type;

    // The payload is the 368 coded bits after the sync word. Decorrelating a
    // soft bit means inverting its confidence where the randomiser sequence
    // has a one.
    auto &payload = scratch->payload;
    for (size_t i = 0; i < payload.size(); i++) {
        size_t index = interleavedIndex(i, payload.size());
        uint16_t value = soft[16 + index];
        if (getBit(sequence, index))
            value = 0xFFFF - value;
        payload[i] = value;
    }

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

void FrameDecoder::decodeLSF(const std::array<uint16_t, 368> &soft)
{
    std::array<uint8_t, sizeof(LinkSetupFrame)> tmp;
    viterbi.decodePunctured(soft, tmp, LSF_PUNCTURE);
    memcpy(&lsf.data, tmp.data(), tmp.size());
}

void FrameDecoder::decodePacket(const std::array<uint16_t, 368> &soft)
{
    // Extract and decode packet data
    std::array<uint8_t, PacketFrame::FRAME_SIZE> tmp;

    viterbi.decodePunctured(soft, tmp, PACKET_PUNCTURE);

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

void FrameDecoder::decodeStream(const std::array<uint16_t, 368> &soft)
{
    // The LICH occupies the first 96 coded bits and is Golay-protected, so it
    // is decoded from hard bits: slice the soft values at half scale.
    lich_t lich;
    lich.fill(0x00);
    for (size_t i = 0; i < lich.size() * 8; i++)
        setBit(lich, i, soft[i] >= 0x8000);

    std::array<uint8_t, 6> lsfSegment;
    bool decodeOk = decodeLich(lsfSegment, lich);
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

    // Extract and decode stream data, which follows the LICH
    auto &stream = scratch->stream;
    std::copy(soft.begin() + lich.size() * 8, soft.end(), stream.begin());

    std::array<uint8_t, sizeof(StreamFrame)> tmp;
    viterbi.decodePunctured(stream, tmp, DATA_PUNCTURE);

    StreamFrame decoded;
    memcpy(&decoded.frameData, tmp.data(), tmp.size());

    // Acceptance check: re-encode the decoded payload as the transmitter did
    // and count the coded bits that differ from the received hard decisions.
    // A decode from noise disagrees with many of them, a genuine frame only
    // where the channel flipped a bit.
    std::array<uint8_t, 34> punctured;
    FrameEncoder::encodeStreamPayload(decoded, punctured);

    uint16_t errors = 0;
    for (size_t i = 0; i < stream.size(); i++) {
        bool received = stream[i] >= 0x8000;
        if (received != getBit(punctured, i))
            errors++;
    }
    streamBitErrors = errors;

    // A rejected payload is not copied, so the previous stream frame is kept.
    if (errors < MAX_STREAM_BIT_ERRORS)
        streamFrame = decoded;
}

bool FrameDecoder::decodeLich(std::array<uint8_t, 6> &segment,
                              const lich_t &lich)
{
    /*
     * Extract and unpack the LICH segment contained in the frame header.
     * The LICH segment is composed of four blocks of Golay(24,12) encoded data
     * and carries five bytes of the original Link Setup Frame. The sixth byte
     * is the segment number, allowing to determine the correct position of the
     * segment when reassembling the LSF.
     *
     * NOTE: LICH data is stored in big-endian format, swap and shift after
     * memcpy convert it to little-endian.
     */

    segment.fill(0x00);

    size_t index = 0;
    uint32_t block = 0;

    for (size_t i = 0; i < 4; i++) {
        memcpy(&block, lich.data() + 3 * i, 3);
        block = __builtin_bswap32(block) >> 8;
        uint16_t decoded = golay24_decode(block);

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
