/***************************************************************************
 *   Copyright (C) 2020 - 2022 by Federico Amedeo Izzo IU2NUO,             *
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <state.h>
#include <battery.h>
#include <hwconfig.h>
#include <interfaces/platform.h>
#include <interfaces/nvmem.h>

state_t state;
pthread_mutex_t state_mutex;

void state_init()
{
    pthread_mutex_init(&state_mutex, NULL);

    /*
     * Try loading settings from nonvolatile memory and default to sane values
     * in case of failure.
     */
    if(nvm_readSettings(&state.settings) < 0)
    {
        state.settings = default_settings;
        strncpy(state.settings.callsign, "OPNRTX", 10);
    }

    if (state.settings.color_bg.r == 0 &&
        state.settings.color_bg.g == 0 &&
        state.settings.color_bg.b == 0 &&
        state.settings.color_bg.alpha == 0) {
        state.settings.color_bg = (color_t) {0, 0, 0, 255};
    }

    /*
     * Try loading VFO configuration from nonvolatile memory and default to sane
     * values in case of failure.
     */
    if(nvm_readVFOChannelData(&state.channel) < 0)
    {
        state.channel = cps_getDefaultChannel();
    }

    /*
     * Initialise remaining fields
     */
    #ifdef RTC_PRESENT
    state.time = rtc_getTime();
    #endif
    state.v_bat  = platform_getVbat();
    state.charge = battery_getCharge(state.v_bat);
    state.rssi   = -127.0f;

    state.channel_index = 1;    // Set default channel index (it is 1-based)
    state.bank_enabled  = false;
    state.rtxStatus     = RTX_OFF;
    state.emergency     = false;

    // Force brightness field to be in range 0 - 100
    if(state.settings.brightness > 100) state.settings.brightness = 100;
}

void state_terminate()
{
    /*
     * Never store a brightness of 0 to avoid booting with a black screen
     */
    if(state.settings.brightness == 0)
    {
        state.settings.brightness = 5;
    }

    nvm_writeSettingsAndVfo(&state.settings, &state.channel);
    pthread_mutex_destroy(&state_mutex);
}

void state_update()
{
    pthread_mutex_lock(&state_mutex);

    /*
     * Low-pass filtering with a time constant of 10s when updated at 1Hz
     * Original computation: state.v_bat = 0.02*vbat + 0.98*state.v_bat
     * Peak error is 18mV when input voltage is 49mV.
     */
    uint16_t vbat = platform_getVbat();
    state.v_bat  -= (state.v_bat * 2) / 100;
    state.v_bat  += (vbat * 2) / 100;

    state.charge = battery_getCharge(state.v_bat);
    state.rssi = rtx_getRssi();

    #ifdef RTC_PRESENT
    state.time = rtc_getTime();
    #endif

    pthread_mutex_unlock(&state_mutex);
}

void state_resetSettingsAndVfo()
{
    state.settings = default_settings;
    state.channel  = cps_getDefaultChannel();
}
