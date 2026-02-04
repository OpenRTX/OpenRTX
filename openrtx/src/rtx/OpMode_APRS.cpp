/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "interfaces/platform.h"
#include "interfaces/radio.h"
#include "rtx/OpMode_APRS.hpp"
#include "rtx/rtx.h"

#include <errno.h>

OpMode_APRS::OpMode_APRS() : rxAudioPath(0), basebandId(0), streamRunning(false)
{
}

OpMode_APRS::~OpMode_APRS()
{
}

void OpMode_APRS::enable()
{
    basebandBuffer = std::make_unique<int16_t[]>(BASEBAND_SIZE);
    demodulator.init();
    streamRunning = false;

    // Descriptors submitted while the mode was off
    cancelRx();
}

void OpMode_APRS::disable()
{
    radio_disableRtx();

    if (streamRunning) {
        audioStream_terminate(basebandId);
        audioPath_release(rxAudioPath);
        streamRunning = false;
    }

    demodulator.terminate();
    basebandBuffer.reset();

    // Queued receive descriptors would otherwise stay submitted to a mode
    // that is gone, and their owner could never hand them to the next one.
    cancelRx();

    platform_ledOff(GREEN);
    platform_ledOff(RED);
}

void OpMode_APRS::cancelRx()
{
    struct pktDesc *pkt;
    int ret;

    do {
        ret = rxPktQueue.tryPop(pkt);
        if (ret == 0) {
            pkt->res = -ECANCELED;
            pkt->status = PKT_STATUS_ERROR;
        }
    } while (ret != -EAGAIN);
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
            if (demodulator.getDCD())
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
    radio_enableRx();
    rxAudioPath = audioPath_request(SOURCE_RTX, SINK_MCU, PRIO_RX);
    basebandId = audioStream_start(rxAudioPath, basebandBuffer.get(),
                                   BASEBAND_SIZE, APRS_SAMPLE_RATE,
                                   STREAM_INPUT | BUF_CIRC_DOUBLE);
    if (basebandId >= 0) {
        status->opStatus = RX;
        streamRunning = true;
        return;
    }

    radio_disableRtx();
    audioPath_release(rxAudioPath);

    // Retry in 30ms, without locking up the other tasks meanwhile
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

        // With no descriptor to fill the receiver keeps running, and the
        // latest frame waits in the demodulator for the next descriptor
        if (rxPktQueue.tryPop(rxPkt) != 0)
            return;

        ssize_t len = demodulator.getFrame(rxPkt->buffer, rxPkt->size);
        if (len < 0) {
            rxPkt->size = 0;
            rxPkt->res = len;
            rxPkt->status = PKT_STATUS_ERROR;
        } else {
            rxPkt->res = len;
            rxPkt->status = PKT_STATUS_DONE;
        }
    }
}

int OpMode_APRS::addPacketRx(struct pktDesc *packet)
{
    if (rxPktQueue.tryPush(packet) != 0)
        return -EAGAIN;

    return 0;
}

void OpMode_APRS::stopRx(rtxStatus_t *const status)
{
    radio_disableRtx();
    audioStream_stop(basebandId);
    audioPath_release(rxAudioPath);
    streamRunning = false;
    status->opStatus = OFF;
}
