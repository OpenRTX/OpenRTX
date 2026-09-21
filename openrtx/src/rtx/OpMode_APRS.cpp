/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "interfaces/delays.h"
#include "interfaces/platform.h"
#include "interfaces/radio.h"
#include "rtx/OpMode_APRS.hpp"
#include "rtx/rtx.h"

#include <errno.h>

OpMode_APRS::OpMode_APRS() :
    enterRx(true), streamRunning(false), rxAudioPath(0), basebandId(0),
    currTxPkt(nullptr), txPhase(TxPhase::KEYUP), txDeferStart(0)
{
}

OpMode_APRS::~OpMode_APRS()
{
}

void OpMode_APRS::enable()
{
    basebandBuffer = std::make_unique<int16_t[]>(2 * APRS_BUF_SIZE);
    demodulator.init();
    modulator.init();

    enterRx = true;
    streamRunning = false;
    currTxPkt.store(nullptr, std::memory_order_relaxed);
    txPhase = TxPhase::KEYUP;
    txDeferStart = 0;
}

void OpMode_APRS::disable()
{
    // A transmission in progress cannot be completed once the operating mode
    // is gone, so unkey and tell the submitter rather than leaving its
    // descriptor — and the inbox entry behind it — pending forever.
    modulator.stop();
    finishTx(-ECANCELED);

    radio_disableRtx();

    if (streamRunning) {
        audioStream_terminate(basebandId);
        audioPath_release(rxAudioPath);
        streamRunning = false;
    }

    demodulator.terminate();
    modulator.terminate();

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
        case TX:
            txState(status);
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
    // A pending transmission does not need the receive path to have come up
    // first: gating TX on a working RX stream would wedge the outbox in
    // exactly the situation where sending matters.
    if (startTxIfPending(status))
        return;

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
            streamRunning = true;
            return;
        }

        audioPath_release(rxAudioPath);
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
        } else {
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

    startTxIfPending(status);
}

void OpMode_APRS::stopRx(rtxStatus_t *const status)
{
    radio_disableRtx();
    if (streamRunning) {
        audioStream_stop(basebandId);
        audioPath_release(rxAudioPath);
        streamRunning = false;
    }
    status->opStatus = OFF;
}

bool OpMode_APRS::startTxIfPending(rtxStatus_t *const status)
{
    struct pktDesc *pkt = currTxPkt.load(std::memory_order_acquire);
    if (pkt == nullptr) {
        txDeferStart = 0;
        return false;
    }

    // Packet TX has no PTT behind it, so pttDisable does not apply: the
    // submitter's call to addPacketTx() is the authorization. The hard RF
    // lockout is a different thing entirely and is always respected.
    if (status->txDisable) {
        finishTx(-EPERM);
        return false;
    }

    if (!channelClear())
        return false;

    // Receiving and transmitting cannot share the baseband path, so the RX
    // stream comes down before the radio is keyed and offState() rebuilds it
    // afterwards.
    if (streamRunning) {
        audioStream_terminate(basebandId);
        audioPath_release(rxAudioPath);
        streamRunning = false;
    }

    demodulator.reset();
    txPhase = TxPhase::KEYUP;
    txDeferStart = 0;
    status->opStatus = TX;

    return true;
}

bool OpMode_APRS::channelClear()
{
    if (!demodulator.DCD()) {
        txDeferStart = 0;
        return true;
    }

    const long long now = getTick();

    if (txDeferStart == 0) {
        txDeferStart = now;
        return false;
    }

    // A channel that never goes quiet, or a demodulator holding carrier
    // detect on noise, must not wedge the outbox: transmit anyway once the
    // deferral has gone on long enough to be a fault rather than courtesy.
    return (now - txDeferStart) >= TX_DEFER_TIMEOUT;
}

void OpMode_APRS::txState(rtxStatus_t *const status)
{
    struct pktDesc *pkt = currTxPkt.load(std::memory_order_acquire);

    // The descriptor can only disappear here if disable() cancelled it.
    if (pkt == nullptr) {
        enterRx = true;
        status->opStatus = OFF;
        return;
    }

    const uint8_t *frame = static_cast<const uint8_t *>(pkt->buffer);

    switch (txPhase) {
        case TxPhase::KEYUP:
            radio_disableRtx();
            radio_enableTx();

            if (!modulator.start()) {
                radio_disableRtx();
                finishTx(-EIO);
                enterRx = true;
                status->opStatus = OFF;
                return;
            }

            // Opening flags give the receiving station's squelch and clock
            // recovery time to settle: a frame that starts before they have
            // is simply not decodable, however clean the signal.
            modulator.sendFlags(TX_DELAY_MS);
            txPhase = TxPhase::FRAME;
            return;

        case TxPhase::FRAME:
            modulator.sendFrame(frame, pkt->size);
            txPhase = TxPhase::TAIL;
            return;

        case TxPhase::TAIL:
            modulator.sendFlags(TX_TAIL_MS);
            modulator.stop();
            radio_disableRtx();

            finishTx((ssize_t)pkt->size);

            txPhase = TxPhase::KEYUP;
            enterRx = true;
            status->opStatus = OFF;
            return;
    }
}

void OpMode_APRS::finishTx(ssize_t result)
{
    struct pktDesc *pkt = currTxPkt.load(std::memory_order_acquire);
    if (pkt == nullptr)
        return;

    pkt->res = result;
    pkt->status = (result < 0) ? PKT_STATUS_ERROR : PKT_STATUS_DONE;

    // Released last: the submitter polls status, so the descriptor must be
    // fully written before the slot is handed back.
    currTxPkt.store(nullptr, std::memory_order_release);
}

int OpMode_APRS::addPacketTx(struct pktDesc *packet)
{
    if (packet == NULL)
        return -EINVAL;
    if ((packet->buffer == NULL) || (packet->size == 0)
        || (packet->size > APRS_PACLEN))
        return -EINVAL;

    struct pktDesc *expected = nullptr;
    if (!currTxPkt.compare_exchange_strong(expected, packet,
                                           std::memory_order_acq_rel,
                                           std::memory_order_relaxed))
        return -EBUSY;

    return 0;
}
