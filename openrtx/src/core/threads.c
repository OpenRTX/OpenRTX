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
#include <battery.h>
#include <interfaces/graphics.h>
#include <interfaces/platform.h>
#include <interfaces/delays.h>
#include <interfaces/radio.h>
#include <event.h>
#include <rtx.h>
#include <queue.h>
#include <minmea.h>
#include <string.h>
#include <utils.h>
#include <input.h>
#ifdef HAS_GPS
#include <interfaces/gps.h>
#include <gps.h>
#endif

/* Mutex for concurrent access to state variable */
pthread_mutex_t state_mutex;

/* Mutex for concurrent access to RTX state variable */
pthread_mutex_t rtx_mutex;

/* Mutex to avoid reading keyboard during display update */
pthread_mutex_t display_mutex;

/* Queue for sending and receiving ui update requests */
queue_t ui_queue;

/**
 * \internal Task function in charge of updating the UI.
 */
void *ui_task(void *arg)
{
    (void) arg;

    // RTX needs synchronization
    bool sync_rtx = true;
    rtxStatus_t rtx_cfg;

    // Get initial state local copy
    pthread_mutex_lock(&state_mutex);
    ui_saveState();
    pthread_mutex_unlock(&state_mutex);

    // Keep the splash screen for 1 second
    sleepFor(1u, 0u);

    // Initial GUI draw
    ui_updateGUI();
    gfx_render();

    while(1)
    {
        // Read from the keyboard queue (returns 0 if no message is present)
        // Copy keyboard_t keys from received void * pointer msg
        event_t event;
        event.value = 0;
        (void) queue_pend(&ui_queue, &event.value, true);

        // Lock mutex, read and write state
        pthread_mutex_lock(&state_mutex);
        // React to keypresses and update FSM inside state
        ui_updateFSM(event, &sync_rtx);
        // Update state local copy
        ui_saveState();
        // Unlock mutex
        pthread_mutex_unlock(&state_mutex);

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

        // Redraw GUI based on last state copy
        ui_updateGUI();
        // Lock display mutex and render display
        pthread_mutex_lock(&display_mutex);
        gfx_render();
        pthread_mutex_unlock(&display_mutex);

        // We don't need a delay because we lock on incoming events
        // TODO: Enable self refresh when a continuous visualization is enabled
        // Update UI at ~33 FPS
        //OSTimeDlyHMSM(0u, 0u, 0u, 30u, OS_OPT_TIME_HMSM_STRICT, &os_err);
    }
}

/**
 * \internal Task function for reading and sending keyboard status.
 */
void *kbd_task(void *arg)
{
    (void) arg;

    // Initialize keyboard driver
    kbd_init();

    while(1)
    {
        kbd_msg_t msg;

        pthread_mutex_lock(&display_mutex);
        bool event = input_scanKeyboard(&msg);
        pthread_mutex_unlock(&display_mutex);

        if(event)
        {
            // Send event_t as void * message to use with OSQPost
            event_t event;
            event.type = EVENT_KBD;
            event.payload = msg.value;
            // Send keyboard status in queue
            (void) queue_post(&ui_queue, event.value);
        }

        // Read keyboard state at 40Hz
        sleepFor(0u, 25u);
    }
}

/**
 * \internal Task function in charge of updating the radio state.
 */
void *dev_task(void *arg)
{
    (void) arg;

    while(1)
    {
        // Lock mutex and update internal state
        pthread_mutex_lock(&state_mutex);
        state_update();
        pthread_mutex_unlock(&state_mutex);

        // Signal state update to UI thread
        event_t dev_msg;
        dev_msg.type = EVENT_STATUS;
        dev_msg.payload = 0;
        (void) queue_post(&ui_queue, dev_msg.value);

        // Execute state update thread every 1s
        sleepFor(1u, 0u);
    }
}

/**
 * \internal Task function for RTX management.
 */
void *rtx_task(void *arg)
{
    (void) arg;

    rtx_init(&rtx_mutex);

    while(1)
    {
        rtx_taskFunc();

        // TODO: implement a cleaner shutdown procedure
        if(state.rtxShutdown == true)
        {
            radio_disableRtx();
            platform_ledOff(RED);
            platform_ledOff(GREEN);
            break;
        }
    }

    return NULL;
}

#if defined(HAS_GPS) && !defined(MD3x0_ENABLE_DBG)
/**
 * \internal Task function for parsing GPS data and updating radio state.
 */
void *gps_task(void *arg)
{
    (void) arg;

    if (!gps_detect(5000)) return NULL;

    gps_init(9600);

    while(1)
    {
        pthread_mutex_lock(&state_mutex);
        gps_taskFunc();
        pthread_mutex_unlock(&state_mutex);

        sleepFor(0u, 100u);
    }
}
#endif

/**
 * \internal This function creates all the system tasks and mutexes.
 */
void create_threads()
{
    // Create state mutex
    pthread_mutex_init(&state_mutex, NULL);

    // Create RTX state mutex
    pthread_mutex_init(&rtx_mutex, NULL);

    // Create display mutex
    pthread_mutex_init(&display_mutex, NULL);

    // Create UI event queue
    queue_init(&ui_queue);

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

    // Create Keyboard thread
    pthread_t      kbd_thread;
    pthread_attr_t kbd_attr;

    pthread_attr_init(&kbd_attr);
    pthread_attr_setstacksize(&kbd_attr, KBD_TASK_STKSIZE);
    pthread_create(&kbd_thread, &kbd_attr, kbd_task, NULL);

#if defined(HAS_GPS) && !defined(MD3x0_ENABLE_DBG)
    // Create GPS thread
    pthread_t      gps_thread;
    pthread_attr_t gps_attr;

    pthread_attr_init(&gps_attr);
    pthread_attr_setstacksize(&gps_attr, GPS_TASK_STKSIZE);
    pthread_create(&gps_thread, &gps_attr, gps_task, NULL);
#endif

    // Create state thread
    pthread_t      state_thread;
    pthread_attr_t state_attr;

    pthread_attr_init(&state_attr);
    pthread_attr_setstacksize(&state_attr, DEV_TASK_STKSIZE);
    pthread_create(&state_thread, &state_attr, dev_task, NULL);
}
