/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "interfaces/radio.h"

void radio_init(const rtxStatus_t *rtxState)
{
    (void) rtxState;
}

void radio_terminate()
{

}

void radio_setOpmode(const enum opmode mode)
{
    (void) mode;
}

bool radio_checkRxDigitalSquelch()
{
    return false;
}

void radio_enableAfOutput()
{

}

void radio_disableAfOutput()
{

}

void radio_enableRx()
{

}

void radio_enableTx()
{

}

void radio_disableRtx()
{

}

void radio_updateConfiguration()
{

}

rssi_t radio_getRssi()
{
    return -154.0f;
}

enum opstatus radio_getStatus()
{
    return OFF;
}
