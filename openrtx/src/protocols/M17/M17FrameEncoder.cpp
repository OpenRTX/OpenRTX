/***************************************************************************
 *   Copyright (C) 2022 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
 *                         Frederik Saraci IU2NRO                          *
 *                         Silvano Seva IU2KWO                             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#include <M17/M17CodePuncturing.h>
#include <M17/M17Decorrelator.h>
#include <M17/M17Interleaver.h>
#include <M17/M17FrameEncoder.h>

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

    // Clear all the LICH segments
    for(auto& segment : lichSegments)
    {
        segment.fill(0x00);
    }
}

void M17FrameEncoder::encodeLsf(M17LinkSetupFrame& lsf, frame_t& output)
{
    // Ensure the LSF to be encoded has a valid CRC field
    lsf.updateCrc();

    // Generate the Golay(24,12) LICH segments
    for(size_t i = 0; i < lichSegments.size(); i++)
    {
        lichSegments[i] = lsf.generateLichSegment(i);
    }

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
    streamFrameNumber = (streamFrameNumber + 1) & 0x07FF;
    if(isLast) streamFrame.lastFrame();
    std::copy(payload.begin(), payload.end(), streamFrame.payload().begin());

    // Encode frame
    std::array<uint8_t, 37> encoded;
    encoder.reset();
    encoder.encode(streamFrame.getData(), encoded.data(), sizeof(M17StreamFrame));
    encoded[36] = encoder.flush();

    std::array<uint8_t, 34> punctured;
    puncture(encoded, punctured, DATA_PUNCTURE);

    // Add LICH segment to coded data
    std::array<uint8_t, 46> frame;
    auto it = std::copy(lichSegments[currentLich].begin(),
                        lichSegments[currentLich].end(),
                        frame.begin());
    std::copy(punctured.begin(), punctured.end(), it);

    // Increment LICH counter after copy
    currentLich = (currentLich + 1) % lichSegments.size();

    interleave(frame);
    decorrelate(frame);

    // Copy data to output buffer, prepended with sync word.
    auto oIt = std::copy(STREAM_SYNC_WORD.begin(), STREAM_SYNC_WORD.end(),
                         output.begin());
    std::copy(frame.begin(), frame.end(), oIt);

    return streamFrame.getFrameNumber();
}
