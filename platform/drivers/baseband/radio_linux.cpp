/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "emulator/emulator.h"
#include "interfaces/radio.h"
#include <cstdio>
#include <string>

void radio_init(const rtxStatus_t *rtxState)
{
    (void) rtxState;
    puts("radio_linux: init() called");
}

void radio_terminate()
{
    puts("radio_linux: terminate() called");
}

void radio_setOpmode(const enum opmode mode)
{
    std::string mStr(" ");
    if(mode == OPMODE_NONE) mStr = "NONE";
    if(mode == OPMODE_FM)   mStr = "FM";
    if(mode == OPMODE_DMR)  mStr = "DMR";
    if(mode == OPMODE_M17)  mStr = "M17";

    printf("radio_linux: setting opmode to %s\n", mStr.c_str());
}

bool radio_checkRxDigitalSquelch()
{
//     puts("radio_linux: radio_checkRxDigitalSquelch(), returning 'false'");
    return false;
}

void radio_enableRx()
{
    puts("radio_linux: enableRx() called");
}

void radio_enableTx()
{
    puts("radio_linux: enableTx() called");
}

void radio_disableRtx()
{
    puts("radio_linux: disableRtx() called");
}

void radio_updateConfiguration()
{
    puts("radio_linux: updateConfiguration() called");
}

rssi_t radio_getRssi()
{
    // Commented to reduce verbosity on Linux
    // printf("radio_linux: requested RSSI at freq %d, returning -100dBm\n", rxFreq);
    return static_cast< rssi_t >(emulator_state.RSSI);
}

enum opstatus radio_getStatus()
{
    return OFF;
}
