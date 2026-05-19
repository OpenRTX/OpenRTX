/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "protocols/M17/CodePuncturing.hpp"
#include "protocols/M17/Decorrelator.hpp"
#include "protocols/M17/Interleaver.hpp"
#include "protocols/M17/FrameEncoder.hpp"
#include "protocols/M17/Constants.hpp"

using namespace M17;

FrameEncoder::FrameEncoder() : currentLich(0), streamFrameNumber(0)
{
    reset();
}

FrameEncoder::~FrameEncoder()
{

}

void FrameEncoder::reset()
{
    // Clear counters
    currentLich       = 0;
    streamFrameNumber = 0;
    updateLsf = false;

    // Clear the LSF
    currLsf.clear();
}

void FrameEncoder::encodeLsf(LinkSetupFrame& lsf, frame_t& output)
{
    lsf.updateCrc();
    currLsf = lsf;

    // Encode the LSF, then puncture and decorrelate its data
    std::array<uint8_t, 61> encoded;
    encoder.reset();
    encoder.encode(lsf.getData(), encoded.data(), sizeof(LinkSetupFrame));
    encoded[60] = encoder.flush();

    std::array<uint8_t, 46> punctured;
    puncture(encoded, punctured, LSF_PUNCTURE);
    interleave(punctured);
    decorrelate(punctured);

    // Copy data to output buffer, prepended with sync word.
    auto it = std::copy(LSF_SYNC_WORD.begin(), LSF_SYNC_WORD.end(),
                        output.begin());
    std::copy(punctured.begin(), punctured.end(), it);
}

uint16_t FrameEncoder::encodeStreamFrame(const payload_t& payload,
                                         frame_t& output, const bool isLast)
{
    StreamFrame streamFrame;

    streamFrame.setFrameNumber(streamFrameNumber);
    streamFrameNumber = (streamFrameNumber + 1) & 0x7FFF;
    if(isLast) streamFrame.lastFrame();
    memcpy(streamFrame.data(), payload.data(), payload.size());

    // Encode frame
    std::array<uint8_t, 37> encoded;
    encoder.reset();
    encoder.encode(&streamFrame.frameData, encoded.data(), sizeof(StreamFrame));
    encoded[36] = encoder.flush();

    std::array<uint8_t, 34> punctured;
    puncture(encoded, punctured, DATA_PUNCTURE);

    // Generate LICH segment
    lich_t lich;
    currLsf.generateLichSegment(lich, currentLich);
    currentLich = (currentLich + 1) % NUM_LSF_CHUNKS;
    if((currentLich == 0) && (updateLsf == true)) {
        currLsf = newLsf;
        updateLsf = false;
    }

    // Add LICH segment to coded data
    std::array<uint8_t, 46> frame;
    auto it = std::copy(lich.begin(), lich.end(), frame.begin());
    std::copy(punctured.begin(), punctured.end(), it);

    interleave(frame);
    decorrelate(frame);

    // Copy data to output buffer, prepended with sync word.
    auto oIt = std::copy(STREAM_SYNC_WORD.begin(), STREAM_SYNC_WORD.end(),
                         output.begin());
    std::copy(frame.begin(), frame.end(), oIt);

    return streamFrame.getFrameNumber();
}

void M17::FrameEncoder::encodeEotFrame(M17::frame_t& output)
{
    for(size_t i = 0; i < output.size(); i += 2)
    {
        output[i]     = 0x55;
        output[i + 1] = 0x5D;
    }
}

void FrameEncoder::encodePacketFrame(const PacketFrame& frame,
                                        frame_t& output)
{
    // Encode frame with convolutional encoder.
    // Rate 1/2 coding produces 2*N bytes; +1 for the flush byte.
    static constexpr size_t ENC_SIZE = PacketFrame::FRAME_SIZE * 2 + 1;
    std::array<uint8_t, ENC_SIZE> encoded;
    encoder.reset();
    encoder.encode(&frame.frameData, encoded.data(), PacketFrame::FRAME_SIZE);
    encoded[ENC_SIZE - 1] = encoder.flush();

    std::array<uint8_t, 46> punctured;
    puncture(encoded, punctured, PACKET_PUNCTURE);

    interleave(punctured);
    decorrelate(punctured);

    // Copy data to output buffer, prepended with sync word.
    auto oIt = std::copy(PACKET_SYNC_WORD.begin(), PACKET_SYNC_WORD.end(),
                         output.begin());
    std::copy(punctured.begin(), punctured.end(), oIt);
}
