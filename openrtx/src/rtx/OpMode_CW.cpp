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

OpMode_CW::OpMode_CW() : lastKeyDown(0), wpm(1), keyDown(false), enterRx(false)
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
    enterRx = true;
}

void OpMode_CW::disable()
{
    platform_ledOff(GREEN);
    platform_ledOff(RED);
    radio_disableRtx();
    paddle_terminate();
    keyDown = false;
    enterRx = false;
}

void OpMode_CW::update(rtxStatus_t *const status, const bool newCfg)
{
    if (newCfg) {
        wpm = status->cwWpm;
        cwKeyer_setSpeed(&keyer, wpm);
        // The radio driver restarts a transmission key up on a new
        // configuration
        keyDown = false;
    }

    long long now = getTick();
    bool down = readKey(now);

    if (down && (status->opStatus != TX) && (status->txDisable == 0))
        down = startTx(status);

    if (status->opStatus == TX) {
        if (down != keyDown) {
            radio_setCwKey(down);
            keyDown = down;
        }

        if (down) {
            lastKeyDown = now;
        } else if (now - lastKeyDown > HANG_DITS * keyer.ditTime) {
            radio_disableRtx();
            status->opStatus = OFF;
            enterRx = true;
        }
    }

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

    sleepFor(0u, POLL_MS);
}

bool OpMode_CW::readKey(long long now)
{
    return cwKeyer_update(&keyer, paddle_read(), now);
}

bool OpMode_CW::startTx(rtxStatus_t *const status)
{
    radio_disableRtx();
    radio_enableTx();
    status->opStatus = TX;
    keyDown = false;

    cwKeyer_init(&keyer, wpm);
    long long now = getTick();
    lastKeyDown = now;

    return readKey(now);
}
