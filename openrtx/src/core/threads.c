/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "hwconfig.h"
#include <pthread.h>
#include "core/ui.h"
#include "core/state.h"
#include "core/threads.h"
#include "core/graphics.h"
#include "interfaces/platform.h"
#include "interfaces/delays.h"
#include "interfaces/radio.h"
#include "core/event.h"
#include "rtx/rtx.h"
#include <string.h>
#include "core/utils.h"
#include "core/input.h"
#include "core/backup.h"
#include "core/gps.h"
#include "core/voicePrompts.h"

#if defined(PLATFORM_TTWRPLUS)
#include "pmu.h"
#endif

/* Mutex for concurrent access to RTX state variable */
pthread_mutex_t rtx_mutex;

/**
 * \internal Thread managing user input and UI
 */
void *ui_threadFunc(void *arg)
{
    (void) arg;

    kbd_msg_t   kbd_msg;
    rtxStatus_t rtx_cfg = { 0 };
    bool        sync_rtx = true;
    long long   time     = 0;

    // Load initial state and update the UI
    ui_saveState();
    ui_updateGUI();

    // Keep the splash screen for one second  before rendering the new UI screen
    sleepFor(1u, 0u);
    gfx_render();

    while(state.devStatus != SHUTDOWN)
    {
        time = getTick();

        if(input_scanKeyboard(&kbd_msg))
        {
            ui_pushEvent(EVENT_KBD, kbd_msg.value);
        }

        pthread_mutex_lock(&state_mutex);   // Lock r/w access to radio state
        ui_updateFSM(&sync_rtx);            // Update UI FSM
        ui_saveState();                     // Save local state copy
        pthread_mutex_unlock(&state_mutex); // Unlock r/w access to radio state

        vp_tick();                           // continue playing voice prompts in progress if any.

        // If synchronization needed take mutex and update RTX configuration
        if(sync_rtx)
        {
            pthread_mutex_lock(&rtx_mutex);
            rtx_cfg.opMode      = state.channel.mode;
            rtx_cfg.bandwidth   = state.channel.bandwidth;
            rtx_cfg.rxFrequency = state.channel.rx_frequency;
            rtx_cfg.txFrequency = state.channel.tx_frequency;
            rtx_cfg.txPower     = state.channel.power;
            rtx_cfg.sqlLevel    = state.settings.sqlLevel;
            rtx_cfg.rxToneEn    = state.channel.fm.rxToneEn;
            rtx_cfg.rxTone      = ctcss_tone[state.channel.fm.rxTone];
            rtx_cfg.txToneEn    = state.channel.fm.txToneEn;
            rtx_cfg.txTone      = ctcss_tone[state.channel.fm.txTone];
            rtx_cfg.toneEn      = state.tone_enabled;

            // Enable Tx if channel allows it and we are in UI main screen
            rtx_cfg.txDisable = state.channel.rx_only || state.txDisable;

            // Copy new M17 CAN, source and destination addresses
            rtx_cfg.can = state.settings.m17_can;
            rtx_cfg.canRxEn = state.settings.m17_can_rx;
            strncpy(rtx_cfg.source_address,      state.settings.callsign, 10);
            strncpy(rtx_cfg.destination_address, state.settings.m17_dest, 10);

            pthread_mutex_unlock(&rtx_mutex);

            rtx_configure(&rtx_cfg);
            sync_rtx = false;
        }

        // Update UI and render on screen, if necessary
        if(ui_updateGUI() == true)
        {
            gfx_render();
        }

        // 40Hz update rate for keyboard and UI
        time += 25;
        sleepUntil(time);
    }

    ui_terminate();
    gfx_terminate();

    return NULL;
}

/**
 * \internal Thread managing the device and update the global state variable.
 */
void *main_thread(void *arg)
{
    (void) arg;

    long long time = 0;

    #if defined(CONFIG_GPS)
    const struct gpsDevice *gps = platform_initGps();
    if(gps != NULL)
        state.gpsDetected = true;
    #endif

    while(state.devStatus != SHUTDOWN)
    {
        time = getTick();

        #if defined(PLATFORM_TTWRPLUS)
        pmu_handleIRQ();
        #endif

        // Check if power off is requested
        pthread_mutex_lock(&state_mutex);
        if(platform_pwrButtonStatus() == false)
            state.devStatus = SHUTDOWN;
        pthread_mutex_unlock(&state_mutex);

        // Run GPS task
        #if defined(CONFIG_GPS)
        gps_task(gps);
        #endif

        // Run state update task
        state_task();

        // Run this loop once every 5ms
        time += 5;
        sleepUntil(time);
    }

    return NULL;
}

/**
 * \internal Thread for RTX management.
 */
void *rtx_threadFunc(void *arg)
{
    (void) arg;

    rtx_init(&rtx_mutex);

    while(state.devStatus == RUNNING)
    {
        rtx_task();
    }

    rtx_terminate();

    return NULL;
}

/**
 * \internal This function creates all the system tasks and mutexes.
 */
void create_threads()
{
    // Create RTX state mutex
    pthread_mutex_init(&rtx_mutex, NULL);

    // Create rtx radio thread
    pthread_attr_t rtx_attr;
    pthread_attr_init(&rtx_attr);

    #ifndef __ZEPHYR__
    pthread_attr_setstacksize(&rtx_attr, RTX_THREAD_STKSIZE);
    #else
    void *rtx_thread_stack = malloc(RTX_THREAD_STKSIZE * sizeof(uint8_t));
    pthread_attr_setstack(&rtx_attr, rtx_thread_stack, RTX_THREAD_STKSIZE);
    #endif

    #ifdef _MIOSIX
    // Max priority for RTX thread when running with miosix rtos
    struct sched_param param;
    param.sched_priority = THREAD_PRIO_HIGH;
    pthread_attr_setschedparam(&rtx_attr, &param);
    #endif

    pthread_t rtx_thread;
    pthread_create(&rtx_thread, &rtx_attr, rtx_threadFunc, NULL);

    // Create UI thread
    pthread_attr_t ui_attr;
    pthread_attr_init(&ui_attr);

    #ifndef __ZEPHYR__
    pthread_attr_setstacksize(&ui_attr, UI_THREAD_STKSIZE);
    #else
    void *ui_thread_stack = malloc(UI_THREAD_STKSIZE * sizeof(uint8_t));
    pthread_attr_setstack(&ui_attr, ui_thread_stack, UI_THREAD_STKSIZE);
    #endif

    pthread_t ui_thread;
    pthread_create(&ui_thread, &ui_attr, ui_threadFunc, NULL);
}
