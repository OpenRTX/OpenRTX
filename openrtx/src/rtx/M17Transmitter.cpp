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
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#include <FirFilter.h>
#include <dsp.h>
#include <toneGenerator_MDx.h>
#include <experimental/array>

#include "M17/M17CodePuncturing.h"
#include "M17/M17Decorrelator.h"
#include "M17/M17Interleaver.h"
#include "M17/M17Transmitter.h"

const auto rrc_taps = std::experimental::make_array<float>
(
    -0.009265784007800534,  -0.006136551625729697,  -0.001125978562075172,
     0.004891777252042491,   0.01071805138282269,    0.01505751553351295,
     0.01679337935001369,    0.015256245142156299,   0.01042830577908502,
     0.003031522725559901,  -0.0055333532968188165, -0.013403099825723372,
     -0.018598682349642525, -0.01944761739590459,   -0.015005271935951746,
     -0.0053887880354343935, 0.008056525910253532,   0.022816244158307273,
     0.035513467692208076,   0.04244131815783876,    0.04025481153629372,
     0.02671818654865632,    0.0013810216516704976, -0.03394615682795165,
    -0.07502635967975885,   -0.11540977897637611,   -0.14703962203941534,
    -0.16119995609538576,   -0.14969512896336504,   -0.10610329539459686,
    -0.026921412469634916,   0.08757875030779196,    0.23293327870303457,
     0.4006012210123992,     0.5786324696325503,     0.7528286479934068,
     0.908262741447522,      1.0309661131633199,     1.1095611856548013,
     1.1366197723675815,     1.1095611856548013,     1.0309661131633199,
     0.908262741447522,      0.7528286479934068,     0.5786324696325503,
     0.4006012210123992,     0.23293327870303457,    0.08757875030779196,
    -0.026921412469634916,  -0.10610329539459686,   -0.14969512896336504,
    -0.16119995609538576,   -0.14703962203941534,   -0.11540977897637611,
    -0.07502635967975885,   -0.03394615682795165,    0.0013810216516704976,
     0.02671818654865632,    0.04025481153629372,    0.04244131815783876,
     0.035513467692208076,   0.022816244158307273,   0.008056525910253532,
    -0.0053887880354343935, -0.015005271935951746,  -0.01944761739590459,
    -0.018598682349642525,  -0.013403099825723372,  -0.0055333532968188165,
     0.003031522725559901,   0.01042830577908502,    0.015256245142156299,
     0.01679337935001369,    0.01505751553351295,    0.01071805138282269,
     0.004891777252042491,  -0.001125978562075172,  -0.006136551625729697,
    -0.009265784007800534
);

static constexpr std::array<uint8_t, 2> LSF_SYNC_WORD  = {0x55, 0xF7};
static constexpr std::array<uint8_t, 2> DATA_SYNC_WORD = {0xFF, 0x5D};

M17Transmitter::M17Transmitter() : currentLich(0), frameNumber(0)
{

}

M17Transmitter::~M17Transmitter()
{

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
    type.CAN      = 0xA;  // Channel access number

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

    // Finally, send preamble and then LSF
    sendPreamble();
    sendData(LSF_SYNC_WORD, punctured);
}

void M17Transmitter::send(const payload_t& payload)
{
    dataFrame.clear();
    dataFrame.setFrameNumber(frameNumber);
    frameNumber = (frameNumber + 1) & 0x07FF;
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

    interleave(frame);
    decorrelate(frame);
    sendData(DATA_SYNC_WORD, frame);
}

void M17Transmitter::stop(const payload_t& payload)
{
    dataFrame.clear();
    dataFrame.setFrameNumber(frameNumber);
    frameNumber = (frameNumber + 1) & 0x07FF;
    dataFrame.lastFrame();
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

    interleave(frame);
    decorrelate(frame);
    sendData(DATA_SYNC_WORD, frame);
}

void M17Transmitter::sendPreamble()
{
    std::array<uint8_t, 2>               preamble_sync;
    std::array<uint8_t, M17_FRAME_SIZE>  preamble_bytes;
    preamble_sync.fill(0x77);
    preamble_bytes.fill(0x77);

    encodeSymbols(preamble_sync, preamble_bytes);
    generateBaseband();
}

void M17Transmitter::sendData(const std::array<uint8_t, 2>& sync,
                              const std::array<uint8_t, M17_FRAME_SIZE>& frame)
{
    encodeSymbols(sync, frame);
    generateBaseband();
}

void M17Transmitter::encodeSymbols(const std::array<uint8_t, 2>& sync,
                                   const std::array<uint8_t, M17_FRAME_SIZE>& frame)
{
    auto sync1 = byteToSymbols(sync[0]);
    auto sync2 = byteToSymbols(sync[1]);

    auto it = std::copy(sync1.begin(), sync1.end(), symbols.begin());
         it = std::copy(sync2.begin(), sync2.end(), it);

    for(size_t i = 0; i < frame.size(); i++)
    {
        auto sym = byteToSymbols(frame[i]);
              it = std::copy(sym.begin(), sym.end(), it);
    }
}

void M17Transmitter::generateBaseband()
{
    using namespace mobilinkd;

    static BaseFirFilter< float, std::tuple_size< decltype(rrc_taps) >::value >
    rrc = mobilinkd::makeFirFilter(rrc_taps);

    idle_outBuffer.fill(0);
    for (size_t i = 0; i != symbols.size(); ++i)
    {
        idle_outBuffer.at(i * 10) = symbols.at(i);
    }

    for(auto& b : idle_outBuffer)
    {
        b = rrc(b) * 7168.0;
    }

    dsp_pwmCompensate(idle_outBuffer.data(), idle_outBuffer.size());

    // Invert phase
    dsp_invertPhase(idle_outBuffer.data(), idle_outBuffer.size());

    for(size_t i = 0; i < M17_FRAME_SAMPLES; i++)
    {
        int32_t pos_sample = idle_outBuffer.at(i) + 32768;
        uint16_t shifted_sample = pos_sample >> 8;
        idle_outBuffer.at(i) = shifted_sample;
    }

    toneGen_waitForStreamEnd();
    std::swap(idle_outBuffer, active_outBuffer);

    toneGen_playAudioStream(reinterpret_cast< uint16_t *>(active_outBuffer.data()),
                       active_outBuffer.size(), M17_RTX_SAMPLE_RATE);
}



