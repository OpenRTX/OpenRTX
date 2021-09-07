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

#include <interfaces/audio_stream.h>
#include <interfaces/audio_path.h>
#include <interfaces/platform.h>
#include <interfaces/delays.h>
#include <interfaces/audio.h>
#include <interfaces/radio.h>
#include <kernel/queue.h>
#include <OpMode_M17.h>
#include <codec2.h>
#include <miosix.h>
#include <rtx.h>
#include <dsp.h>
#include <vector>

using namespace miosix;

using encoded_t = std::array<uint8_t, 8>;
Queue< encoded_t, 2 > audioQueue;
Thread *c2thread;


void c2Func(void *arg)
{
    struct CODEC2 *codec2     = codec2_create(CODEC2_MODE_3200);
    stream_sample_t *audioBuf = new stream_sample_t[320];
    streamId micId = inputStream_start(SOURCE_MIC, PRIO_TX, audioBuf, 320,
                                       BUF_CIRC_DOUBLE, 8000);

    encoded_t encoded;

    while(!Thread::testTerminate())
    {
        dataBlock_t audio = inputStream_getData(micId);

        if(audio.data != NULL)
        {
            // Pre-amplification stage
            for(size_t i = 0; i < audio.len; i++) audio.data[i] <<= 3;

            // DC removal
            dsp_dcRemoval(audio.data, audio.len);

            // Post-amplification stage
            for(size_t i = 0; i < audio.len; i++) audio.data[i] *= 20;

            codec2_encode(codec2, encoded.data(), audio.data);
            audioQueue.put(encoded);
        }
    }

    codec2_destroy(codec2);
    inputStream_stop(micId);
}


OpMode_M17::OpMode_M17() : enterRx(true), m17Tx(modulator)
{

}

OpMode_M17::~OpMode_M17()
{
}

void OpMode_M17::enable()
{
    c2thread = Thread::create(c2Func, 16384);
    modulator.init();
    enterRx = true;
}

void OpMode_M17::disable()
{
    c2thread->terminate();
    c2thread->join();
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

    encoded_t data;
    audioQueue.get(data);
    auto it = std::copy(data.begin(), data.end(), dataFrame.begin());
    audioQueue.get(data);
    std::copy(data.begin(), data.end(), it);
    m17Tx.send(dataFrame, lastFrame);
}
