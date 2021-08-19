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

/*
 * This code was partially adapted from m17-mod code by Mobilinkd
 */

#include <interfaces/platform.h>
#include <interfaces/delays.h>
#include <interfaces/radio.h>
#include <interfaces/audio.h>
#include <interfaces/audio_path.h>
#include <interfaces/audio_stream.h>
#include <hwconfig.h>
#include <OpMode_M17.h>
#include <cstring>
#include <rtx.h>
#include <dsp.h>
#include <stdio.h>

#define M17_AUDIO_SIZE        320
#define M17_VOICE_SAMPLE_RATE 8000


//
// /*
//  * Converts 12-bit unsigned values packed into uint16_t into int16_t samples,
//  * perform in-place conversion to save space.
//  */
// void adc_to_audio_stm32(std::array<audio_sample_t, M17_AUDIO_SIZE> *audio)
// {
//     for (int i = 0; i < M17_AUDIO_SIZE; i++)
//     {
//         (*audio)[i] = (*audio)[i] << 3;
//     }
// }
//
// /*
//  * Receives audio from the STM32 ADCs, connected to the radio's mic
//  */
// std::array<audio_sample_t, M17_AUDIO_SIZE> *OpMode_M17::input_audio_stm32()
// {
//     // Get audio chunk from the microphone stream
//     std::array<stream_sample_t, M17_AUDIO_SIZE> *stream =
//         inputStream_getData<M17_AUDIO_SIZE>(input_id);
//     std::array<audio_sample_t, M17_AUDIO_SIZE> *audio =
//         reinterpret_cast<std::array<audio_sample_t, M17_AUDIO_SIZE>*>(stream);
//     // Convert 12-bit unsigned values into 16-bit signed
//     adc_to_audio_stm32(audio);
//     // Apply DC removal filter
//     dsp_dcRemoval(audio->data(), audio->size());
//
//     return audio;
// }
//
// /*
//  * Pushes the modulated baseband signal into the RTX sink, to transmit M17
//  */
// void OpMode_M17::output_baseband_stm32(std::array<audio_sample_t, M17_FRAME_SAMPLES> *baseband)
// {
//     // Apply PWM compensation FIR filter
//     dsp_pwmCompensate(baseband->data(), baseband->size());
//     // Invert phase
//     dsp_invertPhase(baseband->data(), baseband->size());
//     // In-place cast to uint8_t packed into uint16_t for PWM
//     baseband_to_pwm_stm32(baseband);
//     std::array<stream_sample_t, M17_FRAME_SAMPLES> *stream =
//         reinterpret_cast<std::array<stream_sample_t, M17_FRAME_SAMPLES>*>(baseband);
//
//     streamId output_id = outputStream_start(SINK_RTX,
//                                             PRIO_TX,
//                                             (stream_sample_t *)stream->data(),
//                                             M17_FRAME_SAMPLES,
//                                             M17_RTX_SAMPLE_RATE);
// }

OpMode_M17::OpMode_M17() : enterRx(true),
                           input(nullptr),
                           tx(nullptr)
{

}

OpMode_M17::~OpMode_M17()
{
}

void OpMode_M17::enable()
{
    // Allocate codec2 encoder
//     codec2 = ::codec2_create(CODEC2_MODE_3200);

    tx = new M17Transmitter;

    // When starting, close squelch and prepare for entering in RX mode.
    enterRx   = true;

    // Start sampling from the microphone
    input = new stream_sample_t[2 * M17_AUDIO_SIZE];
    memset(input, 0x00, 2 * M17_AUDIO_SIZE * sizeof(stream_sample_t));

    input_id = inputStream_start(SOURCE_MIC,
                                 PRIO_TX,
                                 input,
                                 2 * M17_AUDIO_SIZE,
                                 BUF_CIRC_DOUBLE,
                                 M17_VOICE_SAMPLE_RATE);
}

void OpMode_M17::disable()
{
    // Terminate microphone sampling
    inputStream_stop(input_id);

    // Clean shutdown.
    audio_disableAmp();
    audio_disableMic();
    radio_disableRtx();
    enterRx   = false;

    // Deallocate arrays to save space
    delete tx;
    delete[] input;
//     codec2_destroy(codec2);
}

void OpMode_M17::update(rtxStatus_t *const status, const bool newCfg)
{
    (void) newCfg;

    // RX logic
    if(status->opStatus == RX)
    {
        // TODO: Implement M17 Rx
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

        // Entering Tx mode right now, setup transmission
        if(status->opStatus != TX)
        {
            audio_disableAmp();
            radio_disableRtx();

            audio_enableMic();
            radio_enableTx();

            status->opStatus = TX;

            // TODO: Allow destinations different than broadcast
            std::string source_address(status->source_address);
            std::string destination_address;

            tx->start(source_address, destination_address);
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

void OpMode_M17::sendData(bool last)
{

// #if defined(PLATFORM_MDUV3x0) | defined(PLATFORM_MD3x0)
//     std::array<audio_sample_t, M17_AUDIO_SIZE> *audio = input_audio_stm32();
// #elif defined(PLATFORM_LINUX)
//     std::array<audio_sample_t, M17_AUDIO_SIZE> *audio = input_audio_linux();
// #else
// #error M17 protocol is not supported on this platform
// #endif

//     codec2_encode(codec2, &dataFrame.payload()[0], &(audio.data()[0]));
//     codec2_encode(codec2, &dataFrame.payload()[8], &(audio.data()[160]));

    payload_t dataFrame;

    static uint8_t cnt = 0;
    static unsigned int nSeed = 5323;
    for(size_t i = 0; i < dataFrame.size(); i++)
    {
        nSeed = (8253729 * nSeed + 2396403);
        dataFrame[i] = nSeed % 256;//'A' + i;
    }

    dataFrame[0] = cnt++;

    tx->send(dataFrame);
    if(last) tx->stop(dataFrame);
}
