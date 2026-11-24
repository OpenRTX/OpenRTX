/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "interfaces/delays.h"
#include "interfaces/paddle.h"
#include "interfaces/platform.h"
#include "interfaces/radio.h"
#include "rtx/OpMode_CW.hpp"
#include "rtx/rtx.h"

#include "core/settings.h"
#include "core/utils.h"

OpMode_CW::OpMode_CW()
    : lastKeyDown(0)
    , wpm(CW_WPM_MIN)
    , keyDown(false)
    , sidetone(false)
    , enterRx(false)
    , reverse(false)
    , straight(false)
    , rfSqlOpen(false)
    , sqlOpen(false)
    , rxAudioPath(0)
{
    cwKeyer_init(&keyer, wpm);
}

OpMode_CW::~OpMode_CW()
{
}

void OpMode_CW::enable()
{
    paddle_init();
    keyDown = false;
    sidetone = false;
    enterRx = true;
}

void OpMode_CW::disable()
{
    platform_ledOff(GREEN);
    platform_ledOff(RED);
    closeSquelch();
    radio_disableRtx();
    paddle_terminate();
    if (sidetone)
        platform_beepStop();
    keyDown = false;
    sidetone = false;
    enterRx = false;
}

void OpMode_CW::update(rtxStatus_t *const status, const bool newCfg)
{
    if (newCfg) {
        wpm = status->cwWpm;
        reverse = status->cwReverse;
        straight = status->cwStraight;
        cwKeyer_setSpeed(&keyer, wpm);
        // The radio driver restarts a transmission key up on a new
        // configuration
        keyDown = false;
    }

    long long now = getTick();
    bool down = readKey(now);

    if (down && status->cwBreakIn && (status->opStatus != TX)
        && (status->txDisable == 0))
        down = startTx(status);

    // Sidetone, following the keyer also when it does not transmit
    if (down != sidetone) {
        if (down)
            platform_beepStart(SIDETONE_HZ);
        else
            platform_beepStop();
        sidetone = down;
    }

    if (status->opStatus == TX) {
        if (down != keyDown) {
            radio_setCwKey(down);
            keyDown = down;
        }

        if (down)
            lastKeyDown = now;

        // Break-in switched off, or the key up for longer than the hang time
        if ((status->cwBreakIn == 0)
            || (now - lastKeyDown > HANG_DITS * keyer.ditTime)) {
            radio_disableRtx();
            status->opStatus = OFF;
            keyDown = false;
            enterRx = true;
        }
    }

    if (status->opStatus == RX)
        updateSquelch(status);

    if ((status->opStatus == OFF) && enterRx) {
        radio_disableRtx();
        radio_enableRx();
        status->opStatus = RX;
        enterRx = false;
    }

    if (keyDown)
        platform_ledOn(RED);
    else
        platform_ledOff(RED);

    if (rfSqlOpen && (status->opStatus == RX))
        platform_ledOn(GREEN);
    else
        platform_ledOff(GREEN);

    sleepFor(0u, POLL_MS);
}

bool OpMode_CW::readKey(long long now)
{
    uint8_t paddles = paddle_read();

    if (reverse) {
        uint8_t dit = (paddles & CW_PADDLE_DAH) ? CW_PADDLE_DIT : 0;
        uint8_t dah = (paddles & CW_PADDLE_DIT) ? CW_PADDLE_DAH : 0;
        paddles = dit | dah;
    }

    // A straight key keys directly; a mono plug also closes the dah contact
    if (straight)
        return (paddles & CW_PADDLE_DIT) != 0;

    return cwKeyer_update(&keyer, paddles, now);
}

bool OpMode_CW::startTx(rtxStatus_t *const status)
{
    closeSquelch();
    radio_disableRtx();
    radio_enableTx();
    status->opStatus = TX;
    keyDown = false;

    cwKeyer_init(&keyer, wpm);
    long long now = getTick();
    lastKeyDown = now;

    return readKey(now);
}

void OpMode_CW::updateSquelch(const rtxStatus_t *const status)
{
    rssi_t squelch = sLevelToRssi(status->sqlLevel);
    rssi_t rssi = rtx_getRssi();

    if (!rfSqlOpen && (rssi > squelch))
        rfSqlOpen = true;
    if (rfSqlOpen && (rssi < squelch))
        rfSqlOpen = false;

    if (rfSqlOpen && !sqlOpen) {
        rxAudioPath = audioPath_request(SOURCE_RTX, SINK_SPK, PRIO_RX);
        sqlOpen = (rxAudioPath > 0);
    } else if (!rfSqlOpen && sqlOpen) {
        closeSquelch();
    }
}

void OpMode_CW::closeSquelch()
{
    if (sqlOpen)
        audioPath_release(rxAudioPath);

    rfSqlOpen = false;
    sqlOpen = false;
}

bool OpMode_CW::rxSquelchOpen()
{
    return sqlOpen;
}
