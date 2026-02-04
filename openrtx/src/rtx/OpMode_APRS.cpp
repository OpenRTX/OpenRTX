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
    // set up the buffer for RX
    basebandBuffer = std::make_unique<int16_t[]>(2 * APRS_BUF_SIZE);

    // reset the DC block filter
    dsp_resetState(dcBlock);

    // When starting, prepare for entering in RX mode.
    enterRx = true;
}

void OpMode_APRS::disable()
{
    // Remove APRS packets that haven't been pulled
    rtxStatus_t status = rtx_getCurrentStatus();
    if (status.aprsPkts) {
        for (aprsPacket_t *pkt = status.aprsPkts; pkt; pkt = pkt->next)
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

    // RX logic
    if (status->opStatus == RX) {
        // get baseband audio
        dataBlock_t baseband = inputStream_getData(basebandId);

        // apply the DC filter and scale the samples by 4 so our convolution
        // operations don't overflow. This seems to work well with the baseband
        // audio level
        for (size_t i = 0; i < APRS_BUF_SIZE; i++)
            baseband.data[i] = dsp_dcBlockFilter(&dcBlock, baseband.data[i])
                            >> 2;

        // demodulate the data
        const int16_t *demod = demodulator.demodulate(baseband.data);

        // slice the demodulator output
        size_t slicerOutputSize;
        const uint8_t *slicerOutput = slicer.slice(demod, slicerOutputSize);

        // decode the frames
        std::vector<std::vector<uint8_t> *> frames =
            decoder.decode(slicerOutput, slicerOutputSize);

        // save packets from the frames
        for (auto frame : frames) {
            aprsPacket_t *pkt = aprsPktFromFrame(frame->data(), frame->size());
            delete frame;
            status->aprsPkts = aprsPktsInsert(status->aprsPkts, pkt);
            status->aprsPktsSize++;
            status->aprsRecv++;
        }
    } else if ((status->opStatus == OFF) && enterRx) {
        radio_disableRtx();

        radio_enableRx();
        status->opStatus = RX;
        rxAudioPath = audioPath_request(SOURCE_RTX, SINK_MCU, PRIO_RX);
        //TODO: should we throw an error if rxAudioPath doesn't work?
        basebandId = audioStream_start(rxAudioPath, basebandBuffer.get(),
                                       2 * APRS_BUF_SIZE, APRS_SAMPLE_RATE,
                                       STREAM_INPUT | BUF_CIRC_DOUBLE);
        enterRx = false;
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
