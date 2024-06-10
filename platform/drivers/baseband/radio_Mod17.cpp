/***************************************************************************
 *   Copyright (C) 2021 - 2023 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN                      *
 *                                Frederik Saraci IU2NRO                   *
 *                                Silvano Seva IU2KWO                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#include <interfaces/radio.h>
#include <peripherals/gpio.h>
#include <calibInfo_Mod17.h>
#include <hwconfig.h>
#include <MCP4551.h>
#include "../audio/MAX9814.h"

static enum  opstatus      radioStatus;   // Current operating status
extern mod17Calib_t mod17CalData;         // Calibration data


void radio_init(const rtxStatus_t *rtxState)
{
    (void) rtxState;

    radioStatus = OFF;

    mcp4551_setWiper(SOFTPOT_TX, mod17CalData.tx_wiper);
    mcp4551_setWiper(SOFTPOT_RX, mod17CalData.rx_wiper);
}

void radio_terminate()
{
    radioStatus = OFF;
}

void radio_tuneVcxo(const int16_t vhfOffset, const int16_t uhfOffset)
{
    (void) vhfOffset;
    (void) uhfOffset;
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

    mcp4551_setWiper(SOFTPOT_TX, mod17CalData.tx_wiper);
    mcp4551_setWiper(SOFTPOT_RX, mod17CalData.rx_wiper);

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

    mcp4551_setWiper(SOFTPOT_TX, mod17CalData.tx_wiper);
    mcp4551_setWiper(SOFTPOT_RX, mod17CalData.rx_wiper);
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
