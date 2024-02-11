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
                           dataValid(false), extendedCall(false),
                           invertTxPhase(false), invertRxPhase(false)
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
    locked       = false;
    dataValid    = false;
    extendedCall = false;
    startRx      = true;
    startTx      = false;
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
    (void) newCfg;
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

    codec_stop(txAudioPath);
    audioPath_release(txAudioPath);

    if(startRx)
    {
        status->opStatus = RX;
        return;
    }

    if(platform_getPttStatus() && (status->txDisable == 0))
    {
        startTx = true;
        status->opStatus = TX;
        return;
    }

    // Sleep for 30ms if there is nothing else to do in order to prevent the
    // rtx thread looping endlessly and locking up all the other tasks
    sleepFor(0, 30);
}

void OpMode_M17::rxState(rtxStatus_t *const status)
{
    if(startRx)
    {
        demodulator.startBasebandSampling();

        radio_enableRx();

        startRx = false;
    }

    bool newData = demodulator.update(invertRxPhase);
    bool lock    = demodulator.isLocked();

    // Reset frame decoder when transitioning from unlocked to locked state.
    if((lock == true) && (locked == false))
    {
        decoder.reset();
        locked = lock;
    }

    if(locked)
    {
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

                // Retrieve extended callsign data
                streamType_t streamType = lsf.getType();

                if((streamType.fields.encType    == M17_ENCRYPTION_NONE) &&
                   (streamType.fields.encSubType == M17_META_EXTD_CALLSIGN))
                {
                    extendedCall = true;

                    meta_t& meta = lsf.metadata();
                    std::string exCall1 = decode_callsign(meta.extended_call_sign.call1);
                    std::string exCall2 = decode_callsign(meta.extended_call_sign.call2);

                    //
                    // The source callsign only contains the last link when
                    // receiving extended callsign data: in order to always store
                    // the true source of a transmission, we need to store the first
                    // extended callsign in M17_src.
                    //
                    strncpy(status->M17_src,  exCall1.c_str(), 10);
                    strncpy(status->M17_refl, exCall2.c_str(), 10);

                    extendedCall = true;
                }

                // Set source and destination fields.
                // If we have received an extended callsign the src will be the RF link address
                // The M17_src will already be stored from the extended callsign
                strncpy(status->M17_dst, dst.c_str(), 10);

                if(extendedCall)
                    strncpy(status->M17_link, src.c_str(), 10);
                else
                    strncpy(status->M17_src, src.c_str(), 10);

                // Check CAN on RX, if enabled.
                // If check is disabled, force match to true.
                bool canMatch =  (streamType.fields.CAN == status->can)
                              || (status->canRxEn == false);

                // Check if the destination callsign of the incoming transmission
                // matches with ours
                bool callMatch = compareCallsigns(std::string(status->source_address), dst);

                // Open audio path only if CAN and callsign match
                uint8_t pthSts = audioPath_getStatus(rxAudioPath);
                if((pthSts == PATH_CLOSED) && (canMatch == true) && (callMatch == true))
                {
                    rxAudioPath = audioPath_request(SOURCE_MCU, SINK_SPK, PRIO_RX);
                    pthSts = audioPath_getStatus(rxAudioPath);
                }

                // Extract audio data and sent it to codec
                if((type == M17FrameType::STREAM) && (pthSts == PATH_OPEN))
                {
                    // (re)start codec2 module if not already up
                    if(codec_running() == false)
                        codec_startDecode(rxAudioPath);

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
        extendedCall  = false;
        status->M17_link[0] = '\0';
        status->M17_refl[0] = '\0';

        codec_stop(rxAudioPath);
        audioPath_release(rxAudioPath);
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

bool OpMode_M17::compareCallsigns(const std::string& localCs,
                                  const std::string& incomingCs)
{
    if((incomingCs == "ALL") || (incomingCs == "INFO") || (incomingCs == "ECHO"))
        return true;

    std::string truncatedLocal(localCs);
    std::string truncatedIncoming(incomingCs);

    int slashPos = localCs.find_first_of('/');
    if(slashPos <= 2)
        truncatedLocal = localCs.substr(slashPos + 1);

    slashPos = incomingCs.find_first_of('/');
    if(slashPos <= 2)
        truncatedIncoming = incomingCs.substr(slashPos + 1);

    if(truncatedLocal == truncatedIncoming)
        return true;

    return false;
}
