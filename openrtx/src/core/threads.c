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

#include <hwconfig.h>
#include <pthread.h>
#include <sched.h>
#include <ui.h>
#include <state.h>
#include <threads.h>
#include <interfaces/graphics.h>
#include <interfaces/platform.h>
#include <interfaces/delays.h>
#include <interfaces/radio.h>
#include <event.h>
#include <rtx.h>
#include <string.h>
#include <utils.h>
#include <input.h>
#ifdef GPS_PRESENT
#include <interfaces/gps.h>
#include <gps.h>
#endif

/* Mutex for concurrent access to state variable */
pthread_mutex_t state_mutex;

/* Mutex for concurrent access to RTX state variable */
pthread_mutex_t rtx_mutex;

/**
 * \internal Task function in charge of updating the UI.
 */
void *ui_task(void *arg)
{
    (void) arg;

    bool        sync_rtx = true;
    rtxStatus_t rtx_cfg;
    kbd_msg_t   kbd_msg;

    // Initialize keyboard driver
    kbd_init();

    // Get initial state local copy
    ui_saveState();

    // Keep the splash screen for 1 second
    sleepFor(1u, 0u);

    // Initial GUI draw
    ui_updateGUI();
    gfx_render();

    while(1)
    {
        // Scan keyboard
        bool kbd_event = input_scanKeyboard(&kbd_msg);
        if(kbd_event)
        {
            event_t event;
            event.type    = EVENT_KBD;
            event.payload = kbd_msg.value;
            ui_pushEvent(event);
        }

        // Lock mutex, read and write state
        pthread_mutex_lock(&state_mutex);
        // React to keypresses and update FSM inside state
        ui_updateFSM(&sync_rtx);
        // Update state local copy
        ui_saveState();
        // Unlock mutex
        pthread_mutex_unlock(&state_mutex);

        // Redraw GUI based on last state copy
        ui_updateGUI();
        gfx_render();

        // If synchronization needed take mutex and update RTX configuration
        if(sync_rtx)
        {
            float power = dBmToWatt(state.channel.power);

            pthread_mutex_lock(&rtx_mutex);
            rtx_cfg.opMode      = state.channel.mode;
            rtx_cfg.bandwidth   = state.channel.bandwidth;
            rtx_cfg.rxFrequency = state.channel.rx_frequency;
            rtx_cfg.txFrequency = state.channel.tx_frequency;
            rtx_cfg.txPower     = power;
            rtx_cfg.sqlLevel    = state.settings.sqlLevel;
            rtx_cfg.rxToneEn    = state.channel.fm.rxToneEn;
            rtx_cfg.rxTone      = ctcss_tone[state.channel.fm.rxTone];
            rtx_cfg.txToneEn    = state.channel.fm.txToneEn;
            rtx_cfg.txTone      = ctcss_tone[state.channel.fm.txTone];

            // Copy new M17 source and destination addresses
            strncpy(rtx_cfg.source_address,      state.settings.callsign, 10);
            strncpy(rtx_cfg.destination_address, state.m17_data.dst_addr, 10);

            pthread_mutex_unlock(&rtx_mutex);

            rtx_configure(&rtx_cfg);
            sync_rtx = false;
        }

        // 40Hz update rate for keyboard and UI
        sleepFor(0, 25);
    }
}

/**
 * \internal Task function in charge of updating the radio state.
 */
void *dev_task(void *arg)
{
    (void) arg;

    #if defined(GPS_PRESENT) && !defined(MD3x0_ENABLE_DBG)
    bool gpsPresent = gps_detect(5000);

    pthread_mutex_lock(&state_mutex);
    state.gpsDetected = gpsPresent;
    pthread_mutex_unlock(&state_mutex);

    if(state.gpsDetected) gps_init(9600);
    #endif

    while(state.shutdown == false)
    {
        // Lock mutex and update internal state
        pthread_mutex_lock(&state_mutex);
        state_update();
        pthread_mutex_unlock(&state_mutex);

        #if defined(GPS_PRESENT) && !defined(MD3x0_ENABLE_DBG)
        if(state.gpsDetected)
        {
            pthread_mutex_lock(&state_mutex);
            gps_taskFunc();
            pthread_mutex_unlock(&state_mutex);
        }
        #endif

        // Signal state update to UI thread
        event_t dev_msg;
        dev_msg.type = EVENT_STATUS;
        dev_msg.payload = 0;
        ui_pushEvent(dev_msg);

        // 10Hz update rate
        sleepFor(0u, 100u);
    }

    #if defined(GPS_PRESENT)
    gps_terminate();
    #endif

    return NULL;
}

/**
 * \internal Task function for RTX management.
 */
void *rtx_task(void *arg)
{
    (void) arg;

    rtx_init(&rtx_mutex);

    while(state.shutdown == false)
    {
        rtx_taskFunc();
    }

    rtx_terminate();

    return NULL;
}

/**
 * \internal This function creates all the system tasks and mutexes.
 */
void create_threads()
{
    // Create state mutex
    pthread_mutex_init(&state_mutex, NULL);

    // Create RTX state mutex
    pthread_mutex_init(&rtx_mutex, NULL);

    // Create rtx radio thread
    pthread_t      rtx_thread;
    pthread_attr_t rtx_attr;

    pthread_attr_init(&rtx_attr);
    pthread_attr_setstacksize(&rtx_attr, RTX_TASK_STKSIZE);

    #ifdef _MIOSIX
    // Max priority for RTX thread when running with miosix rtos
    struct sched_param param;
    param.sched_priority = sched_get_priority_max(0);
    pthread_attr_setschedparam(&rtx_attr, &param);
    #endif

    pthread_create(&rtx_thread, &rtx_attr, rtx_task, NULL);

    // Create state thread
    pthread_t      state_thread;
    pthread_attr_t state_attr;

    pthread_attr_init(&state_attr);
    pthread_attr_setstacksize(&state_attr, DEV_TASK_STKSIZE);
    pthread_create(&state_thread, &state_attr, dev_task, NULL);
}
