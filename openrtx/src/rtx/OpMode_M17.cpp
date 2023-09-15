/***************************************************************************
 *   Copyright (C) 2021 - 2023 by Federico Amedeo Izzo IU2NUO,             *
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
#include <M17/M17Callsign.hpp>
#include <OpMode_M17.hpp>
#include <audio_codec.h>
#include <errno.h>
#include <rtx.h>

#ifdef PLATFORM_MOD17
#include <calibInfo_Mod17.h>
#include <interfaces/platform.h>

extern mod17Calib_t mod17CalData;
#endif

using namespace std;
using namespace M17;

OpMode_M17::OpMode_M17() : startRx(false), startTx(false), locked(false),
                           dataValid(false), invertTxPhase(false),
                           invertRxPhase(false)
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
    locked    = false;
    dataValid = false;
    startRx   = true;
    startTx   = false;
}

void OpMode_M17::disable()
{
    startRx = false;
    startTx = false;
    platform_ledOff(GREEN);
    platform_ledOff(RED);
    audioPath_release(rxAudioPath);
    audioPath_release(txAudioPath);
    codec_terminate();
    radio_disableRtx();
    modulator.terminate();
    demodulator.terminate();
}

void OpMode_M17::update(rtxStatus_t *const status, const bool newCfg)
{
    //
    // FIXME: workaround to avoid UI glitches when a new dst callsign is set.
    //
    // When a new dst callsign is set, the rtx configuration data structure is
    // updated and this may trigger false setting of the lsfOk variable to true,
    // causing the M17 info screen to appear for a very small, but noticeable,
    // amount of time.
    //
    if(newCfg)
        status->lsfOk = false;

    #if defined(PLATFORM_MD3x0) || defined(PLATFORM_MDUV3x0)
    //
    // Invert TX phase for all MDx models.
    // Invert RX phase for MD-3x0 VHF and MD-UV3x0 radios.
    //
    const hwInfo_t* hwinfo = platform_getHwInfo();
    invertTxPhase = true;
    if(hwinfo->vhf_band == 1)
        invertRxPhase = true;
    else
        invertRxPhase = false;
    #elif defined(PLATFORM_MOD17)
    //
    // Get phase inversion settings from calibration.
    //
    invertTxPhase = (mod17CalData.tx_invert == 1) ? true : false;
    invertRxPhase = (mod17CalData.rx_invert == 1) ? true : false;
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

            if(dataValid)
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

    codec_stop(rxAudioPath);
    codec_stop(txAudioPath);
    audioPath_release(rxAudioPath);
    audioPath_release(txAudioPath);

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
        demodulator.startBasebandSampling();
        demodulator.invertPhase(invertRxPhase);

        radio_enableRx();

        startRx = false;
    }

    bool newData = demodulator.update();
    bool lock    = demodulator.isLocked();

    // Reset frame decoder when transitioning from unlocked to locked state.
    if((lock == true) && (locked == false))
    {
        decoder.reset();
        locked = lock;
    }

    if(locked)
    {
        // Check RX audio path status, open it if necessary
        uint8_t pthSts = audioPath_getStatus(rxAudioPath);
        if(pthSts == PATH_CLOSED)
        {
            rxAudioPath = audioPath_request(SOURCE_MCU, SINK_SPK, PRIO_RX);
            pthSts = audioPath_getStatus(rxAudioPath);
        }

        // Start codec2 module if not already up
        if(codec_running() == false)
            codec_startDecode(rxAudioPath);

        // Process new data
        if(newData)
        {
            auto& frame   = demodulator.getFrame();
            auto  type    = decoder.decodeFrame(frame);
            auto  lsf     = decoder.getLsf();
            status->lsfOk = lsf.valid();

            if(status->lsfOk)
            {
                dataValid = true;

                // Retrieve stream source and destination data
                std::string dst = lsf.getDestination();
                std::string src = lsf.getSource();
                strncpy(status->M17_src, src.c_str(), 10);
                strncpy(status->M17_dst, dst.c_str(), 10);

                // Retrieve extended callsign data
                streamType_t streamType = lsf.getType();

                if((streamType.fields.encType    == M17_ENCRYPTION_NONE) &&
                   (streamType.fields.encSubType == M17_META_EXTD_CALLSIGN))
                {
                    meta_t& meta = lsf.metadata();
                    std::string exCall1 = decode_callsign(meta.extended_call_sign.call1);
                    std::string exCall2 = decode_callsign(meta.extended_call_sign.call2);

                    strncpy(status->M17_orig, exCall1.c_str(), 10);
                    strncpy(status->M17_refl, exCall2.c_str(), 10);
                }

                // Extract audio data
                if((type == M17FrameType::STREAM) && (pthSts == PATH_OPEN))
                {
                    M17StreamFrame sf = decoder.getStreamFrame();
                    codec_pushFrame(sf.payload().data(),     false);
                    codec_pushFrame(sf.payload().data() + 8, false);
                }
            }
        }
    }

    locked = lock;

    if(platform_getPttStatus())
    {
        demodulator.stopBasebandSampling();
        locked = false;
        status->opStatus = OFF;
    }

    // Force invalidation of LSF data as soon as lock is lost (for whatever cause)
    if(locked == false)
    {
        status->lsfOk = false;
        dataValid     = false;
        status->M17_orig[0] = '\0';
        status->M17_refl[0] = '\0';
    }
}

void OpMode_M17::txState(rtxStatus_t *const status)
{
    frame_t m17Frame;

    if(startTx)
    {
        startTx = false;

        std::string src(status->source_address);
        std::string dst(status->destination_address);
        M17LinkSetupFrame lsf;

        lsf.clear();
        lsf.setSource(src);
        if(!dst.empty()) lsf.setDestination(dst);

        streamType_t type;
        type.fields.dataMode = M17_DATAMODE_STREAM;     // Stream
        type.fields.dataType = M17_DATATYPE_VOICE;      // Voice data
        type.fields.CAN      = status->can;             // Channel access number

        lsf.setType(type);
        lsf.updateCrc();

        encoder.reset();
        encoder.encodeLsf(lsf, m17Frame);

        txAudioPath = audioPath_request(SOURCE_MIC, SINK_MCU, PRIO_TX);
        codec_startEncode(txAudioPath);
        radio_enableTx();

        modulator.invertPhase(invertTxPhase);
        modulator.start();
        modulator.send(m17Frame);
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

    encoder.encodeStreamFrame(dataFrame, m17Frame, lastFrame);
    modulator.send(m17Frame);

    if(lastFrame)
    {
        encoder.encodeEotFrame(m17Frame);
        modulator.send(m17Frame);
        modulator.stop();
    }
}
