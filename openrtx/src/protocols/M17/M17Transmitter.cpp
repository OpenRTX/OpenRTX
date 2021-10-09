/***************************************************************************
 *   Copyright (C) 2021 by Federico Amedeo Izzo IU2NUO,                    *
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
 *   As a special exception, if other files instantiate templates or use   *
 *   macros or inline functions from this file, or you compile this file   *
 *   and link it with other works to produce a work based on this file,    *
 *   this file does not by itself cause the resulting work to be covered   *
 *   by the GNU General Public License. However the source code for this   *
 *   file must still be made available in accordance with the GNU General  *
 *   Public License. This exception does not invalidate any other reasons  *
 *   why a work based on this file might be covered by the GNU General     *
 *   Public License.                                                       *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#include <M17/M17CodePuncturing.h>
#include <M17/M17Decorrelator.h>
#include <M17/M17Interleaver.h>
#include <M17/M17Transmitter.h>

static constexpr std::array<uint8_t, 2> LSF_SYNC_WORD  = {0x55, 0xF7};
static constexpr std::array<uint8_t, 2> DATA_SYNC_WORD = {0xFF, 0x5D};

M17Transmitter::M17Transmitter(M17Modulator& modulator) : modulator(modulator),
                                                currentLich(0), frameNumber(0)
{

}

M17Transmitter::~M17Transmitter()
{

}

void M17Transmitter::start(const std::string& src)
{
    // Just call start() with an empty string for destination callsign.
    std::string empty;
    start(src, empty);
}

void M17Transmitter::start(const std::string& src, const std::string& dst)
{
    // Reset LICH and frame counters
    currentLich = 0;
    frameNumber = 0;

    // Fill the Link Setup Frame
    lsf.clear();
    lsf.setSource(src);
    if(!dst.empty()) lsf.setDestination(dst);

    streamType_t type;
    type.stream   = 1;    // Stream
    type.dataType = 2;    // Voice data
    type.CAN      = 0;  // Channel access number

    lsf.setType(type);
    lsf.updateCrc();

    // Generate the Golay(24,12) LICH segments
    for(size_t i = 0; i < lichSegments.size(); i++)
    {
        lichSegments[i] = lsf.generateLichSegment(i);
    }

    // Encode the LSF, then puncture and decorrelate its data
    std::array<uint8_t, 61> encoded;
    encoder.reset();
    encoder.encode(&lsf.getData(), encoded.data(), sizeof(lsf_t));
    encoded[60] = encoder.flush();

    std::array<uint8_t, 46> punctured;
    puncture(encoded, punctured, LSF_puncture);
    interleave(punctured);
    decorrelate(punctured);

    // Send preamble
    std::array<uint8_t, 2>  preamble_sync;
    std::array<uint8_t, 46> preamble_bytes;
    preamble_sync.fill(0x77);
    preamble_bytes.fill(0x77);
    modulator.send(preamble_sync, preamble_bytes);

    // Send LSF
    modulator.send(LSF_SYNC_WORD, punctured);
}

void M17Transmitter::send(const payload_t& payload, const bool isLast)
{
    dataFrame.clear();
    dataFrame.setFrameNumber(frameNumber);
    frameNumber = (frameNumber + 1) & 0x07FF;
    if(isLast) dataFrame.lastFrame();
    std::copy(payload.begin(), payload.end(), dataFrame.payload().begin());

    // Encode frame
    std::array<uint8_t, 37> encoded;
    encoder.reset();
    encoder.encode(&dataFrame.getData(), encoded.data(), sizeof(dataFrame_t));
    encoded[36] = encoder.flush();

    std::array<uint8_t, 34> punctured;
    puncture(encoded, punctured, Audio_puncture);

    // Add LICH segment to coded data and send
    std::array<uint8_t, 46> frame;
    auto it = std::copy(lichSegments[currentLich].begin(),
                        lichSegments[currentLich].end(),
                        frame.begin());
    std::copy(punctured.begin(), punctured.end(), it);

    // Increment LICH counter after copy
    currentLich = (currentLich + 1) % lichSegments.size();

    interleave(frame);
    decorrelate(frame);
    modulator.send(DATA_SYNC_WORD, frame);
}
