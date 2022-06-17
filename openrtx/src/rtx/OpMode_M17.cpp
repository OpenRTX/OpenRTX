/***************************************************************************
 *   Copyright (C) 2021 - 2022 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN                      *
 *                                Frederik Saraci IU2NRO                   *
 *                                Silvano Seva IU2KWO                      *
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

#include <interfaces/platform.h>
#include <interfaces/delays.h>
#include <interfaces/audio.h>
#include <interfaces/radio.h>
#include <OpMode_M17.h>
#include <audio_codec.h>
#include <rtx.h>

using namespace std;
using namespace M17;

OpMode_M17::OpMode_M17() : startRx(false), startTx(false), locked(false),
                           m17Tx(modulator)
{

}

OpMode_M17::~OpMode_M17()
{
    disable();
}

void OpMode_M17::enable()
{
    codec_init();
    modulator.init();
    demodulator.init();
    locked  = false;
    startRx = true;
    startTx = false;
}

void OpMode_M17::disable()
{
    startRx = false;
    startTx = false;
    platform_ledOff(GREEN);
    platform_ledOff(RED);
    codec_terminate();
    audio_disableAmp();
    audio_disableMic();
    radio_disableRtx();
    modulator.terminate();
    demodulator.terminate();
}

void OpMode_M17::update(rtxStatus_t *const status, const bool newCfg)
{
    (void) newCfg;

    // Force inversion of RX phase for MD-3x0 VHF and MD-UV3x0 radios
    #if defined(PLATFORM_MD3x0) || defined(PLATFORM_MDUV3x0)
    const hwInfo_t* hwinfo = platform_getHwInfo();
    status->invertRxPhase |= hwinfo->vhf_band;
    #endif

    // Main FSM logic
    switch(status->opStatus)
    {
        case OFF:
            offState(status);
            break;

        case RX:
            rxState(status);
            break;

        case TX:
            txState(status);
            break;

        default:
            break;
    }

    // Led control logic
    switch(status->opStatus)
    {
        case RX:

            if(locked)
                platform_ledOn(GREEN);
            else
                platform_ledOff(GREEN);

            break;

        case TX:
            platform_ledOff(GREEN);
            platform_ledOn(RED);
            break;

        default:
            platform_ledOff(GREEN);
            platform_ledOff(RED);
            break;
    }
}

void OpMode_M17::offState(rtxStatus_t *const status)
{
    radio_disableRtx();

    audio_disableMic();
    audio_disableAmp();
    codec_stop();

    if(startRx)
    {
        status->opStatus = RX;
    }

    if(platform_getPttStatus() && (status->txDisable == 0))
    {
        startTx = true;
        status->opStatus = TX;
    }
}

void OpMode_M17::rxState(rtxStatus_t *const status)
{
    if(startRx)
    {
        decoder.reset();
        demodulator.startBasebandSampling();
        demodulator.invertPhase(status->invertRxPhase);

        audio_enableAmp();
        codec_startDecode(SINK_SPK);

        radio_enableRx();

        startRx = false;
    }

    bool newData = demodulator.update();
    locked       = demodulator.isLocked();

    if(locked && newData)
    {
        auto& frame = demodulator.getFrame();
        auto type   = decoder.decodeFrame(frame);

        if(type == M17FrameType::STREAM)
        {
            M17StreamFrame sf = decoder.getStreamFrame();
            codec_pushFrame(sf.payload().data(),     false);
            codec_pushFrame(sf.payload().data() + 8, false);
        }
    }

    if(platform_getPttStatus())
    {
        demodulator.stopBasebandSampling();
        locked = false;
        status->opStatus = OFF;
    }
}

void OpMode_M17::txState(rtxStatus_t *const status)
{
    if(startTx)
    {
        audio_enableMic();
        codec_startEncode(SOURCE_MIC);

        radio_enableTx();

        std::string source_address(status->source_address);
        std::string destination_address(status->destination_address);
        m17Tx.start(source_address, destination_address);

        startTx = false;
    }

    payload_t dataFrame;
    bool      lastFrame = false;

    // Wait until there are 16 bytes of compressed speech, then send them
    codec_popFrame(dataFrame.data(),     true);
    codec_popFrame(dataFrame.data() + 8, true);

    if(platform_getPttStatus() == false)
    {
        lastFrame = true;
        startRx   = true;
        status->opStatus = OFF;
    }

    m17Tx.send(dataFrame, lastFrame);
}
