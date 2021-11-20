/***************************************************************************
 *   Copyright (C) 2020 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN,                            *
 *                         Frederik Saraci IU2NRO,                         *
 *                         Silvano Seva IU2KWO                             *
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

#include <stdio.h>
#include <string.h>
#include <state.h>
#include <battery.h>
#include <hwconfig.h>
#include <interfaces/platform.h>
#include <interfaces/nvmem.h>

state_t state;

void state_init()
{
    /*
     * Try loading settings from nonvolatile memory and default to sane values
     * in case of failure.
     */
    if(nvm_readSettings(&state.settings) < 0)
    {
        state.settings = default_settings;
        strncpy(state.settings.callsign, "OPNRTX", 10);
    }

    /*
     * Try loading VFO configuration from nonvolatile memory and default to sane
     * values in case of failure.
     */
    if(nvm_readVFOChannelData(&state.channel) < 0)
    {
        state.channel.mode      = FM;
        state.channel.bandwidth = BW_25;
        state.channel.power     = 1.0;

        // Set initial frequency based on supported bands
        const hwInfo_t* hwinfo  = platform_getHwInfo();
        if(hwinfo->uhf_band)
        {
            state.channel.rx_frequency = 430000000;
            state.channel.tx_frequency = 430000000;
        }
        else if(hwinfo->vhf_band)
        {
            state.channel.rx_frequency = 144000000;
            state.channel.tx_frequency = 144000000;
        }

        state.channel.fm.rxToneEn = 0;
        state.channel.fm.rxTone   = 2; // 71.9Hz
        state.channel.fm.txToneEn = 1;
        state.channel.fm.txTone   = 2; // 71.9Hz
    }

    /*
     * Initialise remaining fields
     */
    state.radioStateUpdated = true;
#ifdef HAS_RTC
    state.time = rtc_getTime();
#endif
    state.v_bat  = platform_getVbat();
    state.charge = battery_getCharge(state.v_bat);
    state.rssi   = rtx_getRssi();

    state.channel_index = 1;    // Set default channel index (it is 1-based)
    state.zone_enabled  = false;
    state.rtxStatus     = RTX_OFF;
    state.emergency     = false;
}

void state_terminate()
{
    /*
     * Never store a brightness of 0 to avoid booting with a black screen
     */
    if(state.settings.brightness == 0)
    {
        state.settings.brightness = 25;
    }

    nvm_writeSettingsAndVfo(&state.settings, &state.channel);
}

curTime_t state_getLocalTime(curTime_t utc_time)
{
    curTime_t local_time = utc_time;
    if(local_time.hour + state.settings.utc_timezone >= 24)
    {
        local_time.hour = local_time.hour - 24 + state.settings.utc_timezone;
        local_time.date += 1;
    }
    else if(local_time.hour + state.settings.utc_timezone < 0)
    {
        local_time.hour = local_time.hour + 24 + state.settings.utc_timezone;
        local_time.date -= 1;
    }
    else
        local_time.hour += state.settings.utc_timezone;
    return local_time;
}

curTime_t state_getUTCTime(curTime_t local_time)
{
    curTime_t utc_time = local_time;
    if(utc_time.hour - state.settings.utc_timezone >= 24)
    {
        utc_time.hour = utc_time.hour - 24 - state.settings.utc_timezone;
        utc_time.date += 1;
    }
    else if(utc_time.hour - state.settings.utc_timezone < 0)
    {
        utc_time.hour = utc_time.hour + 24 - state.settings.utc_timezone;
        local_time.date -= 1;
    }
    else
        utc_time.hour -= state.settings.utc_timezone;
    return utc_time;
}
