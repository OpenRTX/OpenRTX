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
#include <interfaces/radio.h>
#include <interfaces/audio.h>
#include <OpMode_DMR.hpp>
#include <rtx.h>

#include <stdio.h>

OpMode_DMR::OpMode_DMR() : startRx(false), startTx(false), locked(false)
{
}

OpMode_DMR::~OpMode_DMR()
{
    disable();
}

void OpMode_DMR::enable()
{
    locked  = false;
    startRx = true;
    startTx = false;
}

void OpMode_DMR::disable()
{
    startRx = false;
    startTx = false;
    platform_ledOff(GREEN);
    platform_ledOff(RED);
    audio_disableAmp();
    audio_disableMic();
    radio_disableRtx();
}

void OpMode_DMR::update(rtxStatus_t *const status, const bool newCfg)
{
    (void) newCfg;

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
    sleepFor(0u, 30u);
}

void OpMode_DMR::offState(rtxStatus_t *const status)
{
    radio_disableRtx();

    audio_disableMic();
    audio_disableAmp();

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

void OpMode_DMR::rxState(rtxStatus_t *const status)
{
    if(startRx)
    {
        //demodulator.startBasebandSampling();

        audio_enableAmp();
        //codec_startDecode(SINK_SPK);

        radio_enableRx();

        startRx = false;
    }

    // bool newData = demodulator.update();
    // bool lock    = demodulator.isLocked();

    // // Reset frame decoder when transitioning from unlocked to locked state
    // if((lock == true) && (locked == false))
    // {
    //     decoder.reset();
    // }

    // locked = lock;

    if(locked) // && newData)
    {
        // auto& frame = demodulator.getFrame();
        // auto type   = decoder.decodeFrame(frame);
        // bool lsfOk  = decoder.getLsf().valid();

        // if((type == M17FrameType::STREAM) && (lsfOk == true))
        // {
        //     M17StreamFrame sf = decoder.getStreamFrame();
        //     codec_pushFrame(sf.payload().data(),     false);
        //     codec_pushFrame(sf.payload().data() + 8, false);
        // }
    }

    if(platform_getPttStatus())
    {
        //demodulator.stopBasebandSampling();
        locked = false;
        status->opStatus = OFF;
    }
}

void OpMode_DMR::txState(rtxStatus_t *const status)
{
    if(startTx)
    {
        audio_enableMic();
        //codec_startEncode(SOURCE_MIC);

        radio_enableTx();

        // std::string source_address(status->source_address);
        // std::string destination_address(status->destination_address);
        // m17Tx.start(source_address, destination_address);

        startTx = false;
    }

    //payload_t dataFrame;

    // Wait until there are 16 bytes of compressed speech, then send them
    // codec_popFrame(dataFrame.data(),     true);
    // codec_popFrame(dataFrame.data() + 8, true);

    if(platform_getPttStatus() == false)
    {
        startRx   = true;
        status->opStatus = OFF;
    }

    // m17Tx.send(dataFrame, lastFrame);
}
