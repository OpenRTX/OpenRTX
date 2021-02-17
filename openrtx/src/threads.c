/***************************************************************************
 *   Copyright (C) 2020 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
 *                         Frederik Saraci IU2NRO                          *
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

#include <hwconfig.h>
#include <os.h>
#include <ui.h>
#include <state.h>
#include <threads.h>
#include <battery.h>
#include <interfaces/keyboard.h>
#include <interfaces/graphics.h>
#include <interfaces/platform.h>
#include <event.h>
#include <rtx.h>
#include <minmea.h>
#ifdef HAS_GPS
#include <interfaces/gps.h>
#include <gps.h>
#endif

/* Mutex for concurrent access to state variable */
static OS_MUTEX state_mutex;

/* Queue for sending and receiving ui update requests */
static OS_Q ui_queue;

/* Mutex for concurrent access to RTX state variable */
static OS_MUTEX rtx_mutex;

/* Mutex to avoid reading keyboard during display update */
static OS_MUTEX display_mutex;

 /**************************** IMPORTANT NOTE ***********************************
  *                                                                             *
  * Rationale for "xx_TASK_STKSIZE/sizeof(CPU_STK)": uC/OS-III manages task     *
  * stack in terms of CPU_STK elements which, on a 32-bit target, are something *
  * like uint32_t, that is, one CPU_STK elements takes four bytes.              *
  *                                                                             *
  * Now, the majority of the world manages stack in terms of *bytes* and this   *
  * leads to an excessive RAM usage if not properly managed. For example, if    *
  * we have, say, xx_TASK_SIZE = 128 with these odd CPU_STK elements we end     *
  * up eating 128*4 = 512 bytes.                                                *
  * The simple workaround for this is dividing the stack size by sizeof(CPU_STK)*
  *******************************************************************************/

/* UI task control block and stack */
static OS_TCB  ui_tcb;
static CPU_STK ui_stk[UI_TASK_STKSIZE/sizeof(CPU_STK)];

/* Keyboard task control block and stack */
static OS_TCB  kbd_tcb;
static CPU_STK kbd_stk[KBD_TASK_STKSIZE/sizeof(CPU_STK)];

/* Device task control block and stack */
static OS_TCB  dev_tcb;
static CPU_STK dev_stk[DEV_TASK_STKSIZE/sizeof(CPU_STK)];

/* Baseband task control block and stack */
static OS_TCB  rtx_tcb;
static CPU_STK rtx_stk[RTX_TASK_STKSIZE/sizeof(CPU_STK)];

#ifdef HAS_GPS
/* GPS task control block and stack */
static OS_TCB  gps_tcb;
static CPU_STK gps_stk[GPS_TASK_STKSIZE/sizeof(CPU_STK)];
#endif

/**
 * \internal Task function in charge of updating the UI.
 */
static void ui_task(void *arg)
{
    (void) arg;
    OS_ERR os_err;
    OS_MSG_SIZE msg_size = 0;
    rtxStatus_t rtx_cfg;
    // RTX needs synchronization
    bool sync_rtx = true;

    // Get initial state local copy
    OSMutexPend(&state_mutex, 0u, OS_OPT_PEND_BLOCKING, 0u, &os_err);
    OSMutexPost(&state_mutex, OS_OPT_POST_NONE, &os_err);

    // Initial GUI draw
    ui_updateGUI(last_state);
    gfx_render();

    while(1)
    {
        // Read from the keyboard queue (returns 0 if no message is present)
        // Copy keyboard_t keys from received void * pointer msg
        void *msg = OSQPend(&ui_queue, 0u, OS_OPT_PEND_BLOCKING,
                            &msg_size, 0u, &os_err);
        event_t event = ((event_t) msg);

        // Lock mutex, read and write state
        OSMutexPend(&state_mutex, 0u, OS_OPT_PEND_BLOCKING, 0u, &os_err);
        // React to keypresses and update FSM inside state
        ui_updateFSM(event, &sync_rtx);
        // Update state local copy
        ui_saveState();
        // Unlock mutex
        OSMutexPost(&state_mutex, OS_OPT_POST_NONE, &os_err);

        // If synchronization needed take mutex and update RTX configuration
        if(sync_rtx)
        {
            OSMutexPend(&rtx_mutex, 0, OS_OPT_PEND_BLOCKING, NULL, &os_err);
            rtx_cfg.opMode = state.channel.mode;
            rtx_cfg.bandwidth = state.channel.bandwidth;
            rtx_cfg.rxFrequency = state.channel.rx_frequency;
            rtx_cfg.txFrequency = state.channel.tx_frequency;
            rtx_cfg.txPower = state.channel.power;
            rtx_cfg.sqlLevel = state.sqlLevel;
            rtx_cfg.rxToneEn = state.channel.fm.rxToneEn;
            rtx_cfg.rxTone = ctcss_tone[state.channel.fm.rxTone];
            rtx_cfg.txToneEn = state.channel.fm.txToneEn;
            rtx_cfg.txTone = ctcss_tone[state.channel.fm.txTone];
            OSMutexPost(&rtx_mutex, OS_OPT_POST_NONE, &os_err);

            rtx_configure(&rtx_cfg);
            sync_rtx = false;
        }

        // Redraw GUI based on last state copy
        ui_updateGUI();
        // Lock display mutex and render display
        OSMutexPend(&display_mutex, 0u, OS_OPT_PEND_BLOCKING, 0u, &os_err);
        gfx_render();
        OSMutexPost(&display_mutex, OS_OPT_POST_NONE, &os_err);

        // We don't need a delay because we lock on incoming events
        // TODO: Enable self refresh when a continuous visualization is enabled
        // Update UI at ~33 FPS
        //OSTimeDlyHMSM(0u, 0u, 0u, 30u, OS_OPT_TIME_HMSM_STRICT, &os_err);
    }
}

/**
 * \internal Task function for reading and sending keyboard status.
 */
static void kbd_task(void *arg)
{
    (void) arg;
    OS_ERR os_err;

    // Initialize keyboard driver
    kbd_init();

    // Allocate timestamp array
    OS_TICK key_ts[kbd_num_keys];
    OS_TICK now;

    // Allocate bool array to send only one long-press event
    bool long_press_sent[kbd_num_keys];

    // Variable for saving previous and current keyboard status
    keyboard_t prev_keys = 0;
    keyboard_t keys = 0;
    bool long_press = false;
    bool send_event = false;

    while(1)
    {
        // Reset flags and get current time
        long_press = false;
        send_event = false;
        // Lock display mutex and read keyboard status
        OSMutexPend(&display_mutex, 0u, OS_OPT_PEND_NON_BLOCKING, 0u, &os_err);
        keys = kbd_getKeys();
        OSMutexPost(&display_mutex, OS_OPT_POST_NONE, &os_err);
        now = OSTimeGet(&os_err);
        // The key status has changed
        if(keys != prev_keys)
        {
            for(uint8_t k=0; k < kbd_num_keys; k++)
            {
                // Key has been pressed
                if(!(prev_keys & (1 << k)) && (keys & (1 << k)))
                {
                    // Save timestamp
                    key_ts[k] = now;
                    send_event = true;
                    long_press_sent[k] = false;
                }
                // Key has been released
                else if((prev_keys & (1 << k)) && !(keys & (1 << k)))
                {
                    send_event = true;
                }
            }
        }
        // Some key is kept pressed
        else if(keys != 0)
        {
            // Check for saved timestamp to trigger long-presses
            for(uint8_t k=0; k < kbd_num_keys; k++)
            {
                // The key is pressed and its long-press timer is over
                if(keys & (1 << k) && !long_press_sent[k] && (now - key_ts[k]) >= kbd_long_interval)
                {
                    long_press = true;
                    send_event = true;
                    long_press_sent[k] = true;
                }
            }
        }
        if(send_event)
        {
            kbd_msg_t msg;
            msg.long_press = long_press;
            msg.keys = keys;
            // Send event_t as void * message to use with OSQPost
            event_t event;
            event.type = EVENT_KBD;
            event.payload = msg.value;
            // Send keyboard status in queue
            OSQPost(&ui_queue, (void *)event.value, sizeof(event_t),
                    OS_OPT_POST_FIFO + OS_OPT_POST_NO_SCHED, &os_err);
        }
        // Save current keyboard state as previous
        prev_keys = keys;

        // Read keyboard state at 20Hz
        OSTimeDlyHMSM(0u, 0u, 0u, 50u, OS_OPT_TIME_HMSM_STRICT, &os_err);
    }
}

/**
 * \internal Task function in charge of updating the radio state.
 */
static void dev_task(void *arg)
{
    (void) arg;
    OS_ERR os_err;

    while(1)
    {
        // Lock mutex and update internal state
        OSMutexPend(&state_mutex, 0u, OS_OPT_PEND_BLOCKING, 0u, &os_err);

#ifdef HAS_RTC
        state.time = rtc_getTime();
        state_applyTimezone();
#endif
        state.v_bat = platform_getVbat();
        state.charge = battery_getCharge(state.v_bat);
        state.rssi = rtx_getRssi();

        OSMutexPost(&state_mutex, OS_OPT_POST_NONE, &os_err);

        // Signal state update to UI thread
        event_t dev_msg;
        dev_msg.type = EVENT_STATUS;
        dev_msg.payload = 0;
        OSQPost(&ui_queue, (void *)dev_msg.value, sizeof(event_t),
                OS_OPT_POST_FIFO + OS_OPT_POST_NO_SCHED, &os_err);

        // Execute state update thread every 1s
        OSTimeDlyHMSM(0u, 0u, 1u, 0u, OS_OPT_TIME_HMSM_STRICT, &os_err);
    }
}

/**
 * \internal Task function for RTX management.
 */
static void rtx_task(void *arg)
{
    (void) arg;
    OS_ERR os_err;

    rtx_init(&rtx_mutex);

    while(1)
    {
        rtx_taskFunc();
        OSTimeDlyHMSM(0u, 0u, 0u, 30u, OS_OPT_TIME_HMSM_STRICT, &os_err);
    }
}

#ifdef HAS_GPS
/**
 * \internal Task function for parsing GPS data and updating radio state.
 */
static void gps_task(void *arg)
{
    (void) arg;
    OS_ERR os_err;
    char line[MINMEA_MAX_LENGTH*10];

    if (!gps_detect(5000))
        return;

    gps_init(9600);
    // Lock mutex to read internal state
    OSMutexPend(&state_mutex, 0u, OS_OPT_PEND_BLOCKING, 0u, &os_err);
    bool enabled = state.settings.gps_enabled;
    OSMutexPost(&state_mutex, OS_OPT_POST_NONE, &os_err);
    if(enabled)
        gps_enable();
    else
        gps_disable();

    while(1)
    {
        int len = gps_getNmeaSentence(line, MINMEA_MAX_LENGTH*10);
        if(len != -1)
        {
            // Lock mutex and update internal state
            OSMutexPend(&state_mutex, 0u, OS_OPT_PEND_BLOCKING, 0u, &os_err);

            // GPS readout is blocking, no need to delay here
            gps_taskFunc(line, len, &state);

            // Unlock state mutex
            OSMutexPost(&state_mutex, OS_OPT_POST_NONE, &os_err);
        }
    }
}
#endif

/**
 * \internal This function creates all the system tasks and mutexes.
 */
void create_threads()
{
    OS_ERR os_err;

    // Create state mutex
    OSMutexCreate((OS_MUTEX  *) &state_mutex,
                  (CPU_CHAR  *) "State Mutex",
                  (OS_ERR    *) &os_err);

    // Create UI event queue
    OSQCreate((OS_Q      *) &ui_queue,
              (CPU_CHAR  *) "UI event queue",
              (OS_MSG_QTY ) 10,
              (OS_ERR    *) &os_err);

    // Create RTX state mutex
    OSMutexCreate((OS_MUTEX  *) &rtx_mutex,
                  (CPU_CHAR  *) "RTX Mutex",
                  (OS_ERR    *) &os_err);

    // Create display mutex
    OSMutexCreate((OS_MUTEX  *) &display_mutex,
                  (CPU_CHAR  *) "Display Mutex",
                  (OS_ERR    *) &os_err);

    // State initialization, execute before starting all tasks
    state_init();

    // Create rtx radio thread
    OSTaskCreate((OS_TCB     *) &rtx_tcb,
                 (CPU_CHAR   *) "RTX Task",
                 (OS_TASK_PTR ) rtx_task,
                 (void       *) 0,
                 (OS_PRIO     ) 5,
                 (CPU_STK    *) &rtx_stk[0],
                 (CPU_STK     ) 0,
                 (CPU_STK_SIZE) RTX_TASK_STKSIZE/sizeof(CPU_STK),
                 (OS_MSG_QTY  ) 0,
                 (OS_TICK     ) 0,
                 (void       *) 0,
                 (OS_OPT      ) (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR     *) &os_err);

    // Create UI thread
    OSTaskCreate((OS_TCB     *) &ui_tcb,
                 (CPU_CHAR   *) "UI Task",
                 (OS_TASK_PTR ) ui_task,
                 (void       *) 0,
                 (OS_PRIO     ) 10,
                 (CPU_STK    *) &ui_stk[0],
                 (CPU_STK     ) 0,
                 (CPU_STK_SIZE) UI_TASK_STKSIZE/sizeof(CPU_STK),
                 (OS_MSG_QTY  ) 0,
                 (OS_TICK     ) 0,
                 (void       *) 0,
                 (OS_OPT      ) (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR     *) &os_err);

    // Create Keyboard thread
    OSTaskCreate((OS_TCB     *) &kbd_tcb,
                 (CPU_CHAR   *) "Keyboard Task",
                 (OS_TASK_PTR ) kbd_task,
                 (void       *) 0,
                 (OS_PRIO     ) 20,
                 (CPU_STK    *) &kbd_stk[0],
                 (CPU_STK     ) 0,
                 (CPU_STK_SIZE) KBD_TASK_STKSIZE/4,
                 (OS_MSG_QTY  ) 0,
                 (OS_TICK     ) 0,
                 (void       *) 0,
                 (OS_OPT      ) (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR     *) &os_err);

#ifdef HAS_GPS
    // Create GPS thread
    OSTaskCreate((OS_TCB     *) &gps_tcb,
                 (CPU_CHAR   *) "GPS Task",
                 (OS_TASK_PTR ) gps_task,
                 (void       *) 0,
                 (OS_PRIO     ) 25,
                 (CPU_STK    *) &gps_stk[0],
                 (CPU_STK     ) 0,
                 (CPU_STK_SIZE) GPS_TASK_STKSIZE/sizeof(CPU_STK),
                 (OS_MSG_QTY  ) 0,
                 (OS_TICK     ) 0,
                 (void       *) 0,
                 (OS_OPT      ) (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR     *) &os_err);
#endif

    // Create state thread
    OSTaskCreate((OS_TCB     *) &dev_tcb,
                 (CPU_CHAR   *) "Device Task",
                 (OS_TASK_PTR ) dev_task,
                 (void       *) 0,
                 (OS_PRIO     ) 30,
                 (CPU_STK    *) &dev_stk[0],
                 (CPU_STK     ) 0,
                 (CPU_STK_SIZE) DEV_TASK_STKSIZE/sizeof(CPU_STK),
                 (OS_MSG_QTY  ) 0,
                 (OS_TICK     ) 0,
                 (void       *) 0,
                 (OS_OPT      ) (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR     *) &os_err);

}
