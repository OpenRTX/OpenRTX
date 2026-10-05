/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "core/ui.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "core/event.h"
#include "core/state.h"
#include "core/battery.h"
#include "core/vfo.h"
#include "hwconfig.h"
#include "interfaces/platform.h"
#include "interfaces/nvmem.h"
#include "interfaces/delays.h"

state_t state;
pthread_mutex_t state_mutex;
static long long int lastUpdate = 0;

// Commonly used frequency steps, expressed in Hz
const uint32_t freq_steps[] = { 1000,  5000,  6250,  10000, 12500,
                                15000, 20000, 25000, 50000, 100000 };
const size_t n_freq_steps = sizeof(freq_steps) / sizeof(freq_steps[0]);

static void migrateSettings(struct settings *new_settings,
                            const settings_t *old_settings)
{
    new_settings->brightness = old_settings->brightness;
    new_settings->contrast = old_settings->contrast;
    new_settings->voxLevel = old_settings->voxLevel;
    new_settings->utc_timezone = old_settings->utc_timezone;
    new_settings->display_timer = old_settings->display_timer;
    new_settings->vpLevel = old_settings->vpLevel;
    new_settings->vpPhoneticSpell = old_settings->vpPhoneticSpell;
    new_settings->macroMenuLatch = old_settings->macroMenuLatch;
    new_settings->gps_enabled = old_settings->gps_enabled;
    new_settings->gpsSetTime = old_settings->gpsSetTime;
    new_settings->m17_can_rx = old_settings->m17_can_rx;
    new_settings->showBatteryIcon = old_settings->showBatteryIcon;

    memset(new_settings->callsign, 0, sizeof(new_settings->callsign));
    strncpy(new_settings->callsign, old_settings->callsign, 10);
}

static void migrateVfo(channel_t *new_vfo, const channel_t *old_vfo,
                       const settings_t *old_settings)
{
    *new_vfo = cps_getDefaultChannel();

    new_vfo->mode = old_vfo->mode;
    new_vfo->bandwidth = old_vfo->bandwidth;
    new_vfo->rx_only = old_vfo->rx_only;
    new_vfo->m17_can = old_settings->m17_can;
    new_vfo->sqlLevel = old_settings->sqlLevel;
    new_vfo->power = old_vfo->power;
    new_vfo->rx_frequency = old_vfo->rx_frequency;
    new_vfo->tx_frequency = old_vfo->tx_frequency;
    new_vfo->scanList_index = old_vfo->scanList_index;
    new_vfo->groupList_index = old_vfo->groupList_index;
    strncpy(new_vfo->name, old_vfo->name, 32);
    strncpy(new_vfo->descr, old_vfo->descr, 32);
    new_vfo->ch_location = old_vfo->ch_location;
    strncpy(new_vfo->m17_dest, old_settings->m17_dest, 10);
    strncpy(new_vfo->M17_meta_text, old_settings->M17_meta_text, 53);

    switch(new_vfo->mode)
    {
        case OPMODE_M17:
            new_vfo->m17 = old_vfo->m17;
            break;
        case OPMODE_FM:
            new_vfo->fm = old_vfo->fm;
            break;
        case OPMODE_DMR:
            new_vfo->dmr = old_vfo->dmr;
            break;
    }
}

void state_init()
{
    settings_t oldSettings;
    channel_t oldVfo;
    int settingsRead, vfoRead;

    pthread_mutex_init(&state_mutex, NULL);

    settingsRead = nvm_readSettings(&oldSettings);
    vfoRead = nvm_readVfoChannelData(&oldVfo);

    /*
     * Load settings from new location first with a fallback to the old one.
     * TODO: remove in OpenRTX v0.6.x
     */
    if (settings_load(&state.settings) < 0) {
        if (settingsRead == 0)
            migrateSettings(&state.settings, &oldSettings);
        else
            state.settings = default_settings;
    }

    /*
     * Load VFO state from new location first with a fallback to the old one.
     * TODO: remove in OpenRTX v0.6.x
     */
    if (vfo_load(&state.channel) < 0) {
        if ((settingsRead == 0) && (vfoRead == 0))
            migrateVfo(&state.channel, &oldVfo, &oldSettings);
        else
            state.channel = cps_getDefaultChannel();
    }

    /*
     * Initialise remaining fields
     */
#ifdef CONFIG_RTC
    state.time = platform_getCurrentTime();
#endif
    state.v_bat = platform_getVbat();
    state.charge = battery_getCharge(state.v_bat);
    state.rssi = -127.0f;
    state.volume = platform_getVolumeLevel();

    state.channel_index = 0; // Set default channel index (it is 0-based)
    state.bank_enabled = false;
    state.rtxStatus = RTX_OFF;
    state.emergency = false;
    state.txDisable = false;
    state.step_index = 4; // Default frequency step 12.5kHz

    // Force brightness field to be in range 0 - 100
    if (state.settings.brightness > 100) {
        state.settings.brightness = 100;
    }
}

void state_terminate()
{
    // Never store a brightness of 0 to avoid booting with a black screen
    if (state.settings.brightness == 0) {
        state.settings.brightness = 5;
    }

    settings_save(&state.settings);
    vfo_save(&state.channel);
    pthread_mutex_destroy(&state_mutex);
}

void state_task()
{
    // Update radio state once every 100ms
    if ((getTick() - lastUpdate) < 100)
        return;

    lastUpdate = getTick();

    pthread_mutex_lock(&state_mutex);

    /*
     * Low-pass filtering with a time constant of 10s when updated at 1Hz
     * Original computation: state.v_bat = 0.02*vbat + 0.98*state.v_bat
     * Peak error is 18mV when input voltage is 49mV.
     *
     * NOTE: GD77 and DM-1801 already have an hardware low-pass filter on the
     * vbat pin. Adding also the digital one seems to cause more troubles than
     * benefits.
     */
    uint16_t vbat = platform_getVbat();
#if defined(PLATFORM_GD77) || defined(PLATFORM_DM1801)
    state.v_bat = vbat;
#else
    state.v_bat -= (state.v_bat * 2) / 100;
    state.v_bat += (vbat * 2) / 100;
#endif

    /*
     * Update volume level, as a 50% average between previous value and a new
     * read of the knob position. This gives a good reactivity while preventing
     * the volume level to jitter when the knob is not being moved.
     */
    uint16_t vol = platform_getVolumeLevel() + state.volume;
    state.volume = vol / 2;

    state.charge = battery_getCharge(state.v_bat);
    state.rssi = rtx_getRssi();

#ifdef CONFIG_RTC
    state.time = platform_getCurrentTime();
#endif

    pthread_mutex_unlock(&state_mutex);

    ui_pushEvent(EVENT_STATUS, 0);
}

void state_resetSettingsAndVfo()
{
    state.settings = default_settings;
    state.channel = cps_getDefaultChannel();
}
