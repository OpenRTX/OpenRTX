/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "protocols/M17/M17CodePuncturing.hpp"
#include "protocols/M17/M17Decorrelator.hpp"
#include "protocols/M17/M17Interleaver.hpp"
#include "protocols/M17/M17FrameEncoder.hpp"
#include "protocols/M17/M17Constants.hpp"

using namespace M17;

M17FrameEncoder::M17FrameEncoder() : currentLich(0), streamFrameNumber(0)
{
    reset();
}

M17FrameEncoder::~M17FrameEncoder()
{

}

void M17FrameEncoder::reset()
{
    // Clear counters
    currentLich       = 0;
    streamFrameNumber = 0;
    updateLsf = false;

    // Clear the LSF
    currLsf.clear();
}

void M17FrameEncoder::encodeLsf(M17LinkSetupFrame& lsf, frame_t& output)
{
    lsf.updateCrc();
    currLsf = lsf;

    // Encode the LSF, then puncture and decorrelate its data
    std::array<uint8_t, 61> encoded;
    encoder.reset();
    encoder.encode(lsf.getData(), encoded.data(), sizeof(M17LinkSetupFrame));
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

uint16_t M17FrameEncoder::encodeStreamFrame(const payload_t& payload,
                                            frame_t& output, const bool isLast)
{
    M17StreamFrame streamFrame;

    streamFrame.setFrameNumber(streamFrameNumber);
    streamFrameNumber = (streamFrameNumber + 1) & 0x7FFF;
    if(isLast) streamFrame.lastFrame();
    std::copy(payload.begin(), payload.end(), streamFrame.payload().begin());

    // Encode frame
    std::array<uint8_t, 37> encoded;
    encoder.reset();
    encoder.encode(streamFrame.getData(), encoded.data(), sizeof(M17StreamFrame));
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

void M17::M17FrameEncoder::encodeEotFrame(M17::frame_t& output)
{
    for(size_t i = 0; i < output.size(); i += 2)
    {
        output[i]     = 0x55;
        output[i + 1] = 0x5D;
    }
}
