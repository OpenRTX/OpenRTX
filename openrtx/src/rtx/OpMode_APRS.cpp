/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "interfaces/radio.h"
#include "rtx/OpMode_APRS.hpp"
#include "protocols/APRS/packet.h"
#include "protocols/APRS/packet_list.h"
#include "rtx/rtx.h"

OpMode_APRS::OpMode_APRS()
{
}

OpMode_APRS::~OpMode_APRS()
{
}

void OpMode_APRS::enable()
{
    basebandBuffer = std::make_unique<int16_t[]>(2 * APRS_BUF_SIZE);
    demodulator.init();
}

void OpMode_APRS::disable()
{
    radio_disableRtx();
    audioStream_terminate(basebandId);
    audioPath_release(rxAudioPath);

    demodulator.terminate();

    platform_ledOff(GREEN);
    platform_ledOff(RED);
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
            if (demodulator.DCD())
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

void OpMode_APRS::offState(rtxStatus_t *const status)
{
    if (!rxPktQueue.empty())
        enterRx = true;

    if (enterRx) {
        radio_enableRx();
        status->opStatus = RX;
        rxAudioPath = audioPath_request(SOURCE_RTX, SINK_MCU, PRIO_RX);
        basebandId = audioStream_start(rxAudioPath, basebandBuffer.get(),
                                       2 * APRS_BUF_SIZE, APRS_SAMPLE_RATE,
                                       STREAM_INPUT | BUF_CIRC_DOUBLE);
        if (basebandId > 0) {
            enterRx = false;
            return;
        }
    }

    // Sleep for 30ms if there is nothing else to do in order to prevent the
    // rtx thread looping endlessly and locking up all the other tasks
    sleepFor(0, 30);
}

void OpMode_APRS::rxState(rtxStatus_t *const status)
{
    dataBlock_t baseband = inputStream_getData(basebandId);
    if (!baseband.data) {
        stopRx(status);
        return;
    }

    if (demodulator.update(baseband)) {
        struct pktDesc *rxPkt;

        int ret = rxPktQueue.tryPop(rxPkt);
        if (ret < 0) {
            if (ret == -EAGAIN)
                stopRx(status);
            return;
        }

        ssize_t len = demodulator.getFrame(rxPkt->buffer, rxPkt->size);
        if (len < 0) {
            rxPkt->size = 0;
            rxPkt->res = len;
            rxPkt->status = PKT_STATUS_ERROR;
        } else {
            rxPkt->size = len;
            rxPkt->res = 0;
            rxPkt->status = PKT_STATUS_DONE;
        }
    }
}

void OpMode_APRS::stopRx(rtxStatus_t *const status)
{
    radio_disableRtx();
    audioStream_stop(basebandId);
    audioPath_release(rxAudioPath);
    status->opStatus = OFF;
}
