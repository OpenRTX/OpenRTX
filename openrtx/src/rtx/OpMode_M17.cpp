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
#include <OpMode_M17.h>
#include <codec2.h>
#include <memory>
#include <vector>
#include <rtx.h>
#include <dsp.h>

using namespace std;

pthread_t       codecThread;        // Thread running CODEC2
pthread_mutex_t codecMtx;           // Mutex for shared access between codec and rtx threads
pthread_cond_t  codecCv;            // Condition variable for data ready
bool            runCodec;           // Flag signalling that codec is running
bool            newData;            // Flag signalling that new data is available
array< uint8_t, 16 > encodedData;   // Buffer for encoded data

void *threadFunc(void *arg)
{
    (void) arg;

    struct CODEC2 *codec2 = codec2_create(CODEC2_MODE_3200);
    unique_ptr< stream_sample_t > audioBuf(new stream_sample_t[320]);
    streamId micId = inputStream_start(SOURCE_MIC, PRIO_TX,
                                       audioBuf.get(), 320,
                                       BUF_CIRC_DOUBLE, 8000);

    size_t pos = 0;

    while(runCodec)
    {
        dataBlock_t audio = inputStream_getData(micId);

        if(audio.data != NULL)
        {
            #if defined(PLATFORM_MD3x0) || defined(PLATFORM_MDUV3x0)
            // Pre-amplification stage
            for(size_t i = 0; i < audio.len; i++) audio.data[i] <<= 3;

            // DC removal
            dsp_dcRemoval(audio.data, audio.len);

            // Post-amplification stage
            for(size_t i = 0; i < audio.len; i++) audio.data[i] *= 4;
            #endif

            // CODEC2 encodes 160ms of speech into 8 bytes: here we write the
            // new encoded data into a buffer of 16 bytes writing the first
            // half and then the second one, sequentially.
            // Data ready flag is rised once all the 16 bytes contain new data.
            uint8_t *curPos = encodedData.data() + 8*pos;
            codec2_encode(codec2, curPos, audio.data);
            pos++;
            if(pos >= 2)
            {
                pthread_mutex_lock(&codecMtx);
                newData = true;
                pthread_cond_signal(&codecCv);
                pthread_mutex_unlock(&codecMtx);
                pos = 0;
            }
        }
    }

    // Unlock waiting thread(s)
    pthread_mutex_lock(&codecMtx);
    newData = true;
    pthread_cond_signal(&codecCv);
    pthread_mutex_unlock(&codecMtx);

    // Tear down codec and input stream
    codec2_destroy(codec2);
    inputStream_stop(micId);

    return NULL;
}


OpMode_M17::OpMode_M17() : enterRx(true), m17Tx(modulator)
{

}

OpMode_M17::~OpMode_M17()
{
}

void OpMode_M17::enable()
{
    // Start CODEC2 thread
    runCodec = true;
    newData  = false;

    pthread_mutex_init(&codecMtx, NULL);
    pthread_cond_init(&codecCv, NULL);

    pthread_attr_t codecAttr;
    pthread_attr_init(&codecAttr);
    pthread_attr_setstacksize(&codecAttr, 16384);
    pthread_create(&codecThread, &codecAttr, threadFunc, NULL);

    modulator.init();
    demodulator.init();
    enterRx = true;
}

void OpMode_M17::disable()
{
    // Shut down CODEC2 thread and wait until it effectively stops
    runCodec = false;
    pthread_join(codecThread, NULL);
    pthread_mutex_destroy(&codecMtx);
    pthread_cond_destroy(&codecCv);

    enterRx = false;
    modulator.terminate();
    demodulator.terminate();
}

void OpMode_M17::update(rtxStatus_t *const status, const bool newCfg)
{
    (void) newCfg;

    // RX logic
    if(status->opStatus == RX)
    {
        // TODO: Implement M17 Rx
        demodulator.update();
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

    // Wait until there are 16 bytes of compressed speech, then send them
    pthread_mutex_lock(&codecMtx);
    while(newData == false)
    {
        pthread_cond_wait(&codecCv, &codecMtx);
    }
    newData = false;
    pthread_mutex_unlock(&codecMtx);

    std::copy(encodedData.begin(), encodedData.end(), dataFrame.begin());
    m17Tx.send(dataFrame, lastFrame);
}
