/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "interfaces/radio.h"
#include "peripherals/gpio.h"
#include "calibration/calibInfo_Mod17.h"
#include "hwconfig.h"
#include "drivers/baseband/MCP4551.h"
#include "../audio/MAX9814.h"

static enum  opstatus      radioStatus;   // Current operating status
extern mod17Calib_t mod17CalData;         // Calibration data


void radio_init(const rtxStatus_t *rtxState)
{
    (void) rtxState;

    radioStatus = OFF;

    mcp4551_setWiper(&i2c1, SOFTPOT_TX, mod17CalData.tx_wiper);
    mcp4551_setWiper(&i2c1, SOFTPOT_RX, mod17CalData.rx_wiper);
}

void radio_terminate()
{
    radioStatus = OFF;
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
    radioStatus = RX;

    mcp4551_setWiper(&i2c1, SOFTPOT_TX, mod17CalData.tx_wiper);
    mcp4551_setWiper(&i2c1, SOFTPOT_RX, mod17CalData.rx_wiper);

    // Module17 PTT output is open drain. This means that, on MCU side, we have
    // to assert the gpio to bring it to low state.
    if(mod17CalData.ptt_out_level)
        gpio_setPin(PTT_OUT);
    else
        gpio_clearPin(PTT_OUT);
}

void radio_enableTx()
{
    radioStatus = TX;

    mcp4551_setWiper(&i2c1, SOFTPOT_TX, mod17CalData.tx_wiper);
    mcp4551_setWiper(&i2c1, SOFTPOT_RX, mod17CalData.rx_wiper);
    max9814_setGain(mod17CalData.mic_gain);

    if(mod17CalData.ptt_out_level)
        gpio_clearPin(PTT_OUT);
    else
        gpio_setPin(PTT_OUT);
}

void radio_disableRtx()
{
    if(mod17CalData.ptt_out_level)
        gpio_setPin(PTT_OUT);
    else
        gpio_clearPin(PTT_OUT);
}

void radio_updateConfiguration()
{

}

rssi_t radio_getRssi()
{
    return -123.0f;
}

enum opstatus radio_getStatus()
{
    return radioStatus;
}
