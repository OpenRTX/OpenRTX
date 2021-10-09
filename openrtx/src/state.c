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
 *   As a special exception, if other files instantiate templates or use   *
 *   macros or inline functions from this file, or you compile this file   *
 *   and link it with other works to produce a work based on this file,    *
 *   this file does not by itself cause the resulting work to be covered   *
 *   by the GNU General Public License. However the source code for this   *
 *   file must still be made available in accordance with the GNU General  *
 *   Public License. This exception does not invalidate any other reasons  *
 *   why a work based on this file might be covered by the GNU General     *
 *   Public License.                                                       *
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
     * TODO: Read current state parameters from hardware,
     * or initialize them to sane defaults
     */
    state.radioStateUpdated = true;
#ifdef HAS_RTC
    state.time = rtc_getTime();
#endif
    state.v_bat = platform_getVbat();
    state.charge = battery_getCharge(state.v_bat);
    state.rssi = rtx_getRssi();

    // Set default channel index (it is 1-based)
    state.channel_index = 1;
    // Read VFO channel from Flash storage
    // NOTE: Disable reading VFO from flash until persistence is implemented
    //if(nvm_readVFOChannelData(&state.channel) != 0)
    if(1)
    {
        // If the read fails set VFO channel default settings
        state.channel.mode = FM;
        state.channel.bandwidth = BW_25;
        state.channel.power = 1.0;
        const hwInfo_t* hwinfo = platform_getHwInfo();
        // Set initial frequency based on supported bands
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
        state.channel.fm.rxTone = 2; // 71.9Hz
        state.channel.fm.txToneEn = 1;
        state.channel.fm.txTone = 2; // 71.9Hz
    }
    state.zone_enabled = false;
    state.rtxStatus = RTX_OFF;
#ifdef HAS_ABSOLUTE_KNOB // If the radio has an absolute position knob
    state.sqlLevel = platform_getChSelector() - 1;
#else
    // Default Squelch: S3 = 4
    state.sqlLevel = 4;
#endif
    state.voxLevel = 0;

    state.emergency = false;

    // Initialize M17_data
    strncpy(state.m17_data.callsign, "OPNRTX", 10);

    // Read settings from flash memory
    // NOTE: Disable reading VFO from flash until persistence is implemented
    //int valid = nvm_readSettings(&state.settings);
    // Settings in flash memory were not valid, restoring default settings
    //if(valid != 0)
    if(1)
    {
        state.settings = default_settings;
        // NOTE: Settings writing disabled until DFU is implemented
        //nvm_writeSettings(&state.settings);
    }
}

void state_terminate()
{
    // Write settings to flash memory
    // NOTE: Disable writing settings to flash until persistence is implemented
    //nvm_writeSettings(&state.settings);
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
