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

using namespace M17;

static constexpr size_t SYNC_BITS = 2 * SYNCWORD_SYMBOLS;
static constexpr size_t PAYLOAD_BITS = 2 * (FRAME_SYMBOLS - SYNCWORD_SYMBOLS);
static constexpr size_t LICH_BITS = 8 * sizeof(lich_t);
static constexpr size_t LICH_BLOCKS = 4;
static constexpr size_t STREAM_BITS = PAYLOAD_BITS - LICH_BITS;

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
}

FrameType FrameDecoder::decodeFrame(const softFrame_t &soft)
{
    syncw_t syncWord = {};
    for (size_t i = 0; i < SYNC_BITS; i++)
        setBit(syncWord, i, hardBit(soft[i]));

    auto type = getFrameType(syncWord);

    switch (type) {
        case FrameType::LINK_SETUP:
            decodeLSF(soft);
            break;

        case FrameType::STREAM:
            decodeStream(soft);
            break;

        case FrameType::PACKET:
            decodePacket(soft);
            break;

        case FrameType::EOT:
            break; // EOT conveys termination only; no payload to decode.

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

uint16_t FrameDecoder::payloadBit(const softFrame_t &soft, const size_t pos)
{
    size_t index = interleavedIndex(pos, PAYLOAD_BITS);
    uint16_t value = soft[SYNC_BITS + index];

    // Decorrelating a soft bit inverts its confidence
    if (getBit(sequence, index))
        value = UINT16_MAX - value;

    return value;
}

void FrameDecoder::decodeLSF(const softFrame_t &soft)
{
    auto payload = [&soft](size_t pos) { return payloadBit(soft, pos); };

    std::array<uint8_t, sizeof(LinkSetupFrame)> tmp;
    viterbi.decodePunctured<PAYLOAD_BITS>(payload, tmp, LSF_PUNCTURE);
    memcpy(&lsf.data, tmp.data(), tmp.size());
}

void FrameDecoder::decodePacket(const softFrame_t &soft)
{
    auto payload = [&soft](size_t pos) { return payloadBit(soft, pos); };

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

void FrameDecoder::decodeStream(const softFrame_t &soft)
{
    std::array<uint8_t, 6> lsfSegment;
    bool decodeOk = decodeLich(lsfSegment, soft);

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
    auto stream = [&soft](size_t pos) {
        return payloadBit(soft, LICH_BITS + pos);
    };

    std::array<uint8_t, sizeof(StreamFrame)> tmp;
    viterbi.decodePunctured<STREAM_BITS>(stream, tmp, DATA_PUNCTURE);

    StreamFrame decoded;
    memcpy(&decoded.frameData, tmp.data(), tmp.size());

    // Acceptance check: re-encode the decoded payload as the transmitter did
    // and count the coded bits that differ from the received hard decisions.
    // A decode from noise disagrees with many of them, a genuine frame only
    // where the channel flipped a bit.
    std::array<uint8_t, STREAM_BITS / 8> punctured;
    FrameEncoder::encodeStreamPayload(decoded, punctured);

    uint16_t errors = 0;
    for (size_t i = 0; i < STREAM_BITS; i++) {
        bool received = hardBit(stream(i));
        if (received != getBit(punctured, i))
            errors++;
    }

    // A rejected payload is not copied, so the previous stream frame is kept.
    if (errors < MAX_STREAM_BIT_ERRORS)
        streamFrame = decoded;
}

bool FrameDecoder::decodeLich(std::array<uint8_t, 6> &segment,
                              const softFrame_t &soft)
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
    std::array<uint16_t, LICH_BITS / LICH_BLOCKS> block;

    for (size_t i = 0; i < LICH_BLOCKS; i++) {
        for (size_t j = 0; j < block.size(); j++)
            block[j] = payloadBit(soft, block.size() * i + j);

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
