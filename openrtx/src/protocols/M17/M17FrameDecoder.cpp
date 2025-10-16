/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "protocols/M17/M17Golay.hpp"
#include "protocols/M17/M17FrameDecoder.hpp"
#include "protocols/M17/M17Interleaver.hpp"
#include "protocols/M17/M17Decorrelator.hpp"
#include "protocols/M17/M17CodePuncturing.hpp"
#include "protocols/M17/M17Constants.hpp"
#include "protocols/M17/M17Utils.hpp"
#include <algorithm>

using namespace M17;

M17FrameDecoder::M17FrameDecoder()
{
}

M17FrameDecoder::~M17FrameDecoder()
{
}

void M17FrameDecoder::reset()
{
    lsfSegmentMap = 0;
    lsf.clear();
    lsfFromLich.clear();
    streamFrame.clear();
}

M17FrameType M17FrameDecoder::decodeFrame(const frame_t &frame)
{
    std::array<uint8_t, 2> syncWord;
    std::array<uint8_t, 46> data;

    std::copy_n(frame.begin(), 2, syncWord.begin());
    std::copy(frame.begin() + 2, frame.end(), data.begin());

    // Re-correlating data is the same operation as decorrelating
    decorrelate(data);
    deinterleave(data);

    auto type = getFrameType(syncWord);

    switch (type) {
        case M17FrameType::LINK_SETUP:
            decodeLSF(data);
            break;

        case M17FrameType::STREAM:
            decodeStream(data);
            break;

        default:
            break;
    }

    return type;
}

M17FrameType
M17FrameDecoder::getFrameType(const std::array<uint8_t, 2> &syncWord)
{
    // Preamble
    M17FrameType type = M17FrameType::PREAMBLE;
    uint8_t minDistance = hammingDistance(syncWord[0], 0x77)
                        + hammingDistance(syncWord[1], 0x77);

    // Link setup frame
    uint8_t hammDistance = hammingDistance(syncWord[0], LSF_SYNC_WORD[0])
                         + hammingDistance(syncWord[1], LSF_SYNC_WORD[1]);

    if (hammDistance < minDistance) {
        type = M17FrameType::LINK_SETUP;
        minDistance = hammDistance;
    }

    // Stream frame
    hammDistance = hammingDistance(syncWord[0], STREAM_SYNC_WORD[0])
                 + hammingDistance(syncWord[1], STREAM_SYNC_WORD[1]);

    if (hammDistance < minDistance) {
        type = M17FrameType::STREAM;
        minDistance = hammDistance;
    }

    // Check value of minimum hamming distance found, if exceeds the allowed
    // limit consider the frame as of unknown type.
    if (minDistance > MAX_SYNC_HAMM_DISTANCE) {
        type = M17FrameType::UNKNOWN;
    }

    return type;
}

void M17FrameDecoder::decodeLSF(const std::array<uint8_t, 46> &data)
{
    std::array<uint8_t, sizeof(M17LinkSetupFrame)> tmp;

    viterbi.decodePunctured(data, tmp, LSF_PUNCTURE);
    memcpy(&lsf.data, tmp.data(), tmp.size());
}

void M17FrameDecoder::decodeStream(const std::array<uint8_t, 46> &data)
{
    // Extract and unpack the LICH segment contained at beginning of frame
    lich_t lich;
    std::array<uint8_t, 6> lsfSegment;

    std::copy_n(data.begin(), lich.size(), lich.begin());
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

    // Extract and decode stream data
    std::array<uint8_t, 34> punctured;
    std::array<uint8_t, sizeof(M17StreamFrame)> tmp;

    auto begin = data.begin();
    begin += lich.size();
    std::copy(begin, data.end(), punctured.begin());

    // Skip payload copy if BER is too high to avoid audio artifacts
    uint16_t bitErrs = viterbi.decodePunctured(punctured, tmp, DATA_PUNCTURE);
    if (bitErrs < MAX_VITERBI_ERRORS)
        memcpy(&streamFrame.data, tmp.data(), tmp.size());
}

bool M17FrameDecoder::decodeLich(std::array<uint8_t, 6> &segment,
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
