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

#include <interfaces/platform.h>
#include <interfaces/delays.h>
#include <interfaces/audio.h>
#include <interfaces/radio.h>
#include <OpMode_M17.h>
#include <rtx.h>

OpMode_M17::OpMode_M17() : enterRx(true), m17Tx(modulator)
{

}

OpMode_M17::~OpMode_M17()
{
}

void OpMode_M17::enable()
{
    modulator.init();
    enterRx = true;
}

void OpMode_M17::disable()
{
    enterRx = false;
    modulator.terminate();
}

void OpMode_M17::update(rtxStatus_t *const status, const bool newCfg)
{
    (void) newCfg;

    // RX logic
    if(status->opStatus == RX)
    {
        // TODO: Implement M17 Rx
        sleepFor(0u, 30u);
    }
    else if((status->opStatus == OFF) && enterRx)
    {
        radio_disableRtx();

        radio_enableRx();
        status->opStatus = RX;
        enterRx = false;
    }

    // TX logic
    if(platform_getPttStatus() && (status->txDisable == 0))
    {
        // Enter Tx mode, setup transmission
        if(status->opStatus != TX)
        {
            audio_disableAmp();
            radio_disableRtx();

            audio_enableMic();
            radio_enableTx();

            std::string source_address(status->source_address);
            std::string destination_address(status->destination_address);
            m17Tx.start(source_address, destination_address);

            status->opStatus = TX;
        }
        else
        {
            // Transmission is ongoing, just modulate
            sendData(false);
        }
    }

    // PTT is off, transition to Rx state
    if(!platform_getPttStatus() && (status->opStatus == TX))
    {
        // Send last audio frame
        sendData(true);

        audio_disableMic();
        radio_disableRtx();

        status->opStatus = OFF;
        enterRx = true;
    }

    // Led control logic
    switch(status->opStatus)
    {
        case RX:
            // TODO: Implement Rx LEDs
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

void OpMode_M17::sendData(bool lastFrame)
{
    payload_t dataFrame;

    // TODO: temporarily data frame is filled with pseudorandom data
    static unsigned int nSeed = 5323;
    for(size_t i = 0; i < dataFrame.size(); i++)
    {
        nSeed = (8253729 * nSeed + 2396403);
        dataFrame[i] = nSeed % 256;
    }

    m17Tx.send(dataFrame, lastFrame);
}
