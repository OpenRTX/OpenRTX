/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "interfaces/platform.h"
#include "interfaces/delays.h"
#include "interfaces/audio.h"
#include "interfaces/radio.h"
#include "protocols/M17/Datatypes.hpp"
#include "protocols/M17/PacketFrame.hpp"
#include "protocols/M17/PacketDeframer.hpp"
#include "protocols/M17/PacketFramer.hpp"
#include "rtx/SmsTxPacket.hpp"
#include "rtx/OpMode_M17.hpp"
#include "core/audio_codec.h"
#include <errno.h>
#include "core/gps.h"
#include "core/crc.h"
#include "core/state.h"
#include "core/utils.h"
#include "rtx/rtx.h"
#include <cstring>

#ifdef PLATFORM_MOD17
#include "calibration/calibInfo_Mod17.h"
#include "interfaces/platform.h"

extern mod17Calib_t mod17CalData;
#endif

using namespace std;
using namespace M17;

OpMode_M17::OpMode_M17()
    : startRx(false)
    , startTx(false)
    , locked(false)
    , dataValid(false)
    , extendedCall(false)
    , invertTxPhase(false)
    , invertRxPhase(false)
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
    smsQueue.clear();
    locked = false;
    dataValid = false;
    extendedCall = false;
    startRx = true;
    startTx = false;
    pktStarted = false;
    pktDeframer.reset();
    lastCRC = 0;
    pendingSender[0] = '\0';
    state.totalSMSMessages = 0;
    state.havePacketData = false;
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
    smsQueue.clear();
}

bool OpMode_M17::getSMSMessage(uint8_t index, char *sender, size_t sender_len,
                               char *message, size_t message_len)
{
    return smsQueue.get(index, sender, sender_len, message, message_len);
}

void OpMode_M17::delSMSMessage(uint8_t index)
{
    smsQueue.erase(index);
}

void OpMode_M17::update(rtxStatus_t *const status, const bool newCfg)
{
    (void)newCfg;
#if defined(PLATFORM_MD3x0) || defined(PLATFORM_MDUV3x0)
    //
    // Invert TX phase for all MDx models.
    // Invert RX phase for MD-3x0 VHF and MD-UV3x0 radios.
    //
    const hwInfo_t *hwinfo = platform_getHwInfo();
    invertTxPhase = true;
    if (hwinfo->vhf_band == 1)
        invertRxPhase = true;
    else
        invertRxPhase = false;
#elif defined(PLATFORM_MOD17)
    //
    // Get phase inversion settings from calibration.
    //
    invertTxPhase = (mod17CalData.bb_tx_invert == 1) ? true : false;
    invertRxPhase = (mod17CalData.bb_rx_invert == 1) ? true : false;
#elif defined(PLATFORM_CS7000) || defined(PLATFORM_CS7000P)
    invertTxPhase = true;
#elif defined(PLATFORM_DM1701)
    invertTxPhase = true;
    invertRxPhase = true;
#endif

    // Main FSM logic
    switch (status->opStatus) {
        case OFF:
            offState(status);
            break;

        case RX:
            rxState(status);
            break;

        case TX:
            if (state.havePacketData)
                txPacketState(status);
            else
                txState(status);
            break;

        default:
            break;
    }

    // Led control logic
    switch (status->opStatus) {
        case RX:

            if (dataValid)
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

    if (startRx) {
        status->opStatus = RX;
        return;
    }

    if (platform_getPttStatus() && (status->txDisable == 0)) {
        startTx = true;
        status->opStatus = TX;
        return;
    }

    if (state.havePacketData) {
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
    if (startRx) {
        demodulator.startBasebandSampling();

        radio_enableRx();

        startRx = false;
    }

    bool newData = demodulator.update(invertRxPhase);
    bool lock = demodulator.isLocked();

    // Reset frame decoder when transitioning from unlocked to locked state.
    if ((lock == true) && (locked == false)) {
        decoder.reset();
        locked = lock;
    }

    if (locked) {
        // Process new data
        if (newData) {
            auto &frame = demodulator.getFrame();
            auto type = decoder.decodeFrame(frame);
            auto lsf = decoder.getLsf();
            status->lsfOk = lsf.valid();

            if (status->lsfOk) {
                dataValid = true;

                // Retrieve stream source and destination data
                Callsign dst = lsf.getDestination();
                Callsign src = lsf.getSource();
                strncpy(status->M17_dst, dst, 10);

                // Copy source callsign (may be overridden for extended callsigns)
                strncpy(status->M17_src, src, 10);

                // Retrieve extended callsign data
                streamType_t streamType = lsf.getType();

                if (streamType.fields.encType == ENCRYPTION_NONE) {
                    meta_t &meta = lsf.metadata();

                    switch (streamType.fields.encSubType) {
                        case META_EXTD_CALLSIGN: {
                            extendedCall = true;
                            Callsign exCall1(meta.extended_call_sign.call1);
                            Callsign exCall2(meta.extended_call_sign.call2);

                            // The source callsign only contains the last link when
                            // receiving extended callsign data: store the first
                            // extended callsign in M17_src.
                            strncpy(status->M17_src, exCall1, 10);
                            strncpy(status->M17_refl, exCall2, 10);
                            strncpy(status->M17_link, src, 10);
                            break;
                        }
                        case META_TEXT: {
                            metaText.addBlock(meta);
                            const char *txt = metaText.getText();
                            if (txt != nullptr)
                                strncpy(status->M17_meta_text, txt,
                                        sizeof(status->M17_meta_text) - 1);
                            break;
                        }
                        default:
                            // M17_src already set above
                            break;
                    }
                }
                // M17_src already set above for non-encrypted streams

                // Check CAN on RX, if enabled.
                // If check is disabled, force match to true.
                bool canMatch = (streamType.fields.CAN == status->can)
                             || (status->canRxEn == false);

                // Check if the destination callsign of the incoming transmission
                // matches with ours
                bool callMatch = (Callsign(status->source_address) == dst)
                              || dst.isSpecial();

                // Open audio path only if CAN and callsign match
                uint8_t pthSts = audioPath_getStatus(rxAudioPath);
                if ((pthSts == PATH_CLOSED) && (canMatch == true)
                    && (callMatch == true)) {
                    rxAudioPath = audioPath_request(SOURCE_MCU, SINK_SPK,
                                                    PRIO_RX);
                    pthSts = audioPath_getStatus(rxAudioPath);
                }

                // Extract audio data and sent it to codec
                if ((type == FrameType::STREAM) && (pthSts == PATH_OPEN)) {
                    // (re)start codec2 module if not already up
                    if (codec_running() == false)
                        codec_startDecode(rxAudioPath);

                    StreamFrame sf = decoder.getStreamFrame();
                    codec_pushFrame(sf.data(), false);
                    codec_pushFrame(sf.data() + 8, false);
                }
                // Check if packet frame and SMS receive enabled
                else if (type == FrameType::PACKET && (canMatch == true)
                         && ((callMatch == true)
                             || !state.settings.m17_sms_match_call)) {
                    PacketFrame pf = decoder.getPacketFrame();

                    // Start reassembly on first packet frame
                    if (!pktStarted) {
                        if (smsQueue.count() == 0)
                            lastCRC = 0;

                        pktStarted = true;
                        pktDeframer.reset();
                        strncpy(pendingSender, status->M17_src,
                                SMSQueue::CALLSIGN_LEN - 1);
                        pendingSender[SMSQueue::CALLSIGN_LEN - 1] = '\0';
                    }

                    if (pktStarted) {
                        auto result = pktDeframer.pushFrame(pf);

                        if (result == DeframerResult::COMPLETE) {
                            const uint8_t *pktData = pktDeframer.data();
                            size_t pktLen = pktDeframer.length();

                            // Application layer: check protocol ID and
                            // extract SMS text (skip 0x05 prefix byte).
                            if (pktLen > 1 && pktData[0] == 0x05) {
                                // Compute CRC for dedup using the stored
                                // big-endian value from the wire.
                                uint16_t crc = crc_m17(pktData, pktLen);
                                if (crc != lastCRC) {
                                    smsQueue.push(
                                        pendingSender,
                                        reinterpret_cast<const char *>(
                                            &pktData[1]));
                                    lastCRC = crc;
                                    state.totalSMSMessages = smsQueue.count();
                                }
                            }
                            pktStarted = false;
                        } else if (result != DeframerResult::IN_PROGRESS) {
                            // Any error: abort reassembly
                            pktStarted = false;
                        }
                    }
                }
            }
        }
    }

    locked = lock;

    bool shouldExit = platform_getPttStatus();
    shouldExit = shouldExit || state.havePacketData;
    if (shouldExit) {
        demodulator.stopBasebandSampling();
        locked = false;
        status->opStatus = OFF;
    }

    // Force invalidation of LSF data as soon as lock is lost (for whatever cause)
    if (locked == false) {
        status->lsfOk = false;
        dataValid = false;
        extendedCall = false;
        pktStarted = false;
        status->M17_meta_text[0] = '\0';
        status->M17_link[0] = '\0';
        status->M17_refl[0] = '\0';

        metaText.reset();
        codec_stop(rxAudioPath);
        audioPath_release(rxAudioPath);
    }
}

void OpMode_M17::txState(rtxStatus_t *const status)
{
    frame_t m17Frame;

    if (startTx) {
        startTx = false;

        LinkSetupFrame lsf;

        lsf.clear();
        lsf.setSource(status->source_address);

        Callsign dst(status->destination_address);
        if (!dst.isEmpty())
            lsf.setDestination(dst);

        streamType_t type;
        type.fields.dataMode = DATAMODE_STREAM; // Stream
        type.fields.dataType = DATATYPE_VOICE;  // Voice data
        type.fields.CAN = status->can;          // Channel access number

        lsf.setType(type);

        if (strlen(state.settings.M17_meta_text) > 0) {
            metaText.setText(state.settings.M17_meta_text);
            metaText.getNextBlock(lsf.metadata());
        }

        if (state.settings.gps_enabled) {
            lsf.setGnssData(&state.gps_data, GNSS_STATION_HANDHELD);
            gpsTimer = 0;
        }

        encoder.reset();
        encoder.encodeLsf(lsf, m17Frame);

        txAudioPath = audioPath_request(SOURCE_MIC, SINK_MCU, PRIO_TX);
        codec_startEncode(txAudioPath);
        radio_enableTx();

        modulator.invertPhase(invertTxPhase);
        modulator.start();
        modulator.sendPreamble();
        modulator.sendFrame(m17Frame);
    }
    payload_t dataFrame;
    bool lastFrame = false;

    // Wait until there are 16 bytes of compressed speech, then send them
    codec_popFrame(dataFrame.data(), true);
    codec_popFrame(dataFrame.data() + 8, true);

    if (platform_getPttStatus() == false) {
        lastFrame = true;
        startRx = true;
        status->opStatus = OFF;
    }

    encoder.encodeStreamFrame(dataFrame, m17Frame, lastFrame);
    modulator.sendFrame(m17Frame);

    // After encoding a stream frame the encoder advances its LICH counter.
    // When it wraps back to zero a new superframe begins and the encoder
    // will accept an updated LSF.  Schedule the next meta-text block or
    // GPS update at this boundary so the new data is transmitted during
    // the upcoming superframe.
    if (encoder.superframeBoundary()) {
        if (strlen(state.settings.M17_meta_text) > 0) {
            auto lsf = encoder.getCurrentLsf();
            metaText.getNextBlock(lsf.metadata());
            encoder.updateLsfData(lsf);
        }

        if (state.settings.gps_enabled) {
            gpsTimer++;

            if (gpsTimer >= GPS_UPDATE_TICKS) {
                auto lsf = encoder.getCurrentLsf();
                lsf.setGnssData(&state.gps_data, GNSS_STATION_HANDHELD);
                encoder.updateLsfData(lsf);
                gpsTimer = 0;
            }
        }
    }

    if (lastFrame) {
        encoder.encodeEotFrame(m17Frame);
        modulator.sendFrame(m17Frame);
        modulator.stop();
    }
}

void OpMode_M17::txPacketState(rtxStatus_t *const status)
{
    frame_t m17Frame;

    if (locked)
        demodulator.stopBasebandSampling();

    // Check message length
    size_t msgLen = strlen(state.sms_message);
    if (msgLen == 0) {
        state.havePacketData = false;
        startRx = true;
        status->opStatus = OFF;
        return;
    }

    // Format SMS packet data
    size_t appDataLen = prepareSmsPacketData(state.sms_message, msgLen,
                                             pktBuffer, sizeof(pktBuffer));
    if (appDataLen == 0) {
        state.havePacketData = false;
        startRx = true;
        status->opStatus = OFF;
        return;
    }

    // Initialise framer
    pktFramer.init(pktBuffer, appDataLen);

    // Build and send Link Setup Frame
    LinkSetupFrame lsf;
    lsf.clear();
    lsf.setSource(status->source_address);

    Callsign dst(status->destination_address);
    if (!dst.isEmpty())
        lsf.setDestination(dst);

    streamType_t type;
    type.fields.dataMode = DATAMODE_PACKET;
    type.fields.dataType = DATATYPE_DATA;
    type.fields.CAN = status->can;
    lsf.setType(type);

    encoder.reset();
    encoder.encodeLsf(lsf, m17Frame);

    radio_enableTx();

    modulator.invertPhase(invertTxPhase);
    modulator.start();
    modulator.sendPreamble();
    modulator.sendFrame(m17Frame);

    // Send packet frames
    PacketFrame pf;
    while (pktFramer.nextFrame(pf)) {
        encoder.encodePacketFrame(pf, m17Frame);
        modulator.sendFrame(m17Frame);
    }

    // Send EOT
    encoder.encodeEotFrame(m17Frame);
    modulator.sendFrame(m17Frame);
    modulator.stop();

    state.havePacketData = false;
    memset(state.sms_message, 0, sizeof(state.sms_message));
    lastCRC = 0;
    startRx = true;
    status->opStatus = OFF;
}
