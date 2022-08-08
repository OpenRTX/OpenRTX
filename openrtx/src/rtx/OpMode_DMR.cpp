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

using namespace DMR;

OpMode_DMR::OpMode_DMR() : locked(false), startRx(false), startTx(false), dmrCodec()
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
    // switch(status->opStatus)
    // {
    //     case RX:
    //         if(locked)
    //             platform_ledOn(GREEN);
    //         else
    //             platform_ledOff(GREEN);
    //         break;

    //     case TX:
    //         platform_ledOff(GREEN);
    //         platform_ledOn(RED);
    //         break;

    //     default:
    //         platform_ledOff(GREEN);
    //         platform_ledOff(RED);
    //         break;
    // }

    // Sleep thread for 30ms for 33Hz update rate
    sleepFor(0u, 15u);
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
        dmrCodec.stop();
        startTx = true;
        status->opStatus = TX;
    }
}

void OpMode_DMR::rxState(rtxStatus_t *const status)
{
    if(startRx)
    {
        audio_enableAmp();
        radio_enableRx();
        dmrCodec.start(VOCODER_DIR_DECODE);
        startRx = false;
    }

    locked = dmrCodec.isLocked();

    if (locked) {
        // Grab srcId, tgId, TA, data from radio

        // Eventually tie to database for LastHeard
    }

    if(platform_getPttStatus())
    {
        dmrCodec.stop();
        status->opStatus = OFF;
    }
}

void OpMode_DMR::txState(rtxStatus_t *const status)
{
    if(startTx)
    {
        radio_enableTx();
        dmrCodec.start(VOCODER_DIR_ENCODE);
        startTx = false;
    }

    if(platform_getPttStatus() == false)
    {
        dmrCodec.stop();
        startRx   = true;
        status->opStatus = OFF;
    }
}
