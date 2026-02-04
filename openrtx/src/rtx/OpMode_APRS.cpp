/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "interfaces/radio.h"
#include "rtx/OpMode_APRS.hpp"
#include "protocols/APRS/packet.h"
#include "rtx/rtx.h"

OpMode_APRS::OpMode_APRS()
{
}

OpMode_APRS::~OpMode_APRS()
{
}

void OpMode_APRS::enable()
{
    // initialize demodulator
    demodulator.init();

    // set up the buffer for RX
    basebandBuffer = std::make_unique<int16_t[]>(2 * APRS_BUF_SIZE);

    // When starting, prepare for entering in RX mode.
    enterRx = true;
}

void OpMode_APRS::disable()
{
    demodulator.terminate();

    // Remove APRS packets that haven't been pulled
    rtxStatus_t status = rtx_getCurrentStatus();
    if (status.aprsPkts) {
        for (struct aprsPacket *pkt = status.aprsPkts; pkt; pkt = pkt->next)
            aprsPktFree(pkt);
    }

    // Clean shutdown.
    platform_ledOff(GREEN);
    platform_ledOff(RED);

    audioStream_terminate(basebandId);
    audioPath_release(rxAudioPath);

    radio_disableRtx();

    enterRx = true;
}

void OpMode_APRS::update(rtxStatus_t *const status, const bool newCfg)
{
    (void)newCfg;

    // Main FSM logic
    switch (status->opStatus) {
        case OFF:
            offState(status);
            break;
        case RX:
            rxState(status);
            break;
        default:
            break;
    }

    // Led control logic
    switch (status->opStatus) {
        case RX:
            platform_ledOn(GREEN);
            //TODO: we should make the red LED DCD
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

void OpMode_APRS::offState(rtxStatus_t *const status)
{
    if (enterRx) {
        radio_disableRtx();

        radio_enableRx();
        status->opStatus = RX;
        rxAudioPath = audioPath_request(SOURCE_RTX, SINK_MCU, PRIO_RX);
        //TODO: should we throw an error if rxAudioPath doesn't work?
        basebandId = audioStream_start(rxAudioPath, basebandBuffer.get(),
                                       2 * APRS_BUF_SIZE, APRS_SAMPLE_RATE,
                                       STREAM_INPUT | BUF_CIRC_DOUBLE);
        enterRx = false;
        return;
    }

    // Sleep for 30ms if there is nothing else to do in order to prevent the
    // rtx thread looping endlessly and locking up all the other tasks
    sleepFor(0, 30);
}

void OpMode_APRS::rxState(rtxStatus_t *const status)
{
    // get baseband audio
    dataBlock_t baseband = inputStream_getData(basebandId);

    if (demodulator.update(baseband)) {
        // create packet from the frame and save
        frameData &frame = demodulator.getFrame();
        struct aprsPacket *pkt = aprsPktFromFrame(frame.data, frame.len);
        status->aprsPkts = aprsPktsInsert(status->aprsPkts, pkt);
        status->aprsPktsSize++;
        status->aprsRecv++;
    }
}
