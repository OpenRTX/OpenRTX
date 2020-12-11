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

#include <os.h>
#include <ui.h>
#include <state.h>
#include <threads.h>
#include <keyboard.h>
#include <graphics.h>
#include <platform.h>
#include <hwconfig.h>
#include <event.h>

#include <stdio.h>

/* Mutex for concurrent access to state variable */
static OS_MUTEX state_mutex;

/* Queue for sending and receiving ui update requests */
static OS_Q ui_queue;

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

/* DMR task control block and stack */
static OS_TCB  dmr_tcb;
static CPU_STK dmr_stk[DMR_TASK_STKSIZE/sizeof(CPU_STK)];


/**
 * \internal Task function in charge of updating the UI.
 */
static void ui_task(void *arg)
{
    (void) arg;
    OS_ERR os_err;
    OS_MSG_SIZE msg_size = 0;

    // Get initial state local copy
    OSMutexPend(&state_mutex, 0u, OS_OPT_PEND_BLOCKING, 0u, &os_err);
    state_t last_state = state;
    OSMutexPost(&state_mutex, OS_OPT_POST_NONE, &os_err);

    // Initial GUI draw
    ui_updateGUI(last_state);
    gfx_render();
    while(gfx_renderingInProgress());

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
        ui_updateFSM(event);
        // Update state local copy
        last_state = state;
        // Unlock mutex
        OSMutexPost(&state_mutex, OS_OPT_POST_NONE, &os_err);

        // Redraw GUI based on last state copy
        bool renderNeeded = ui_updateGUI(last_state);

        if(renderNeeded)
        {
            gfx_render();
            while(gfx_renderingInProgress());
        }

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

    while(1)
    {
        keyboard_t keys = kbd_getKeys();
		// Check if some key is pressed
        if(keys != 0)
        {
            // Send event_t as void * message to use with OSQPost
            event_t kbd_msg;
            kbd_msg.type = EVENT_KBD;
            kbd_msg.payload = keys;

            // Send keyboard status in queue
            OSQPost(&ui_queue, (void *)kbd_msg.value, sizeof(event_t), 
                    OS_OPT_POST_FIFO + OS_OPT_POST_NO_SCHED, &os_err);
        }
        // Read keyboard state at 5Hz
        OSTimeDlyHMSM(0u, 0u, 0u, 200u, OS_OPT_TIME_HMSM_STRICT, &os_err);
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

        state.time = rtc_getTime(); 
        state.v_bat = platform_getVbat();

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

    while(1)
    {
        // Execute rtx radio thread every 30ms to match DMR task
        //TODO: uncomment after rtx.h merge
        //rtx_main();
        OSTimeDlyHMSM(0u, 0u, 0u, 30u, OS_OPT_TIME_HMSM_STRICT, &os_err);
    }
}

/**
 * \internal Task function for DMR management.
 */
static void dmr_task(void *arg)
{
    (void) arg;
    OS_ERR os_err;

    while(1)
    {
        // Execute dmr radio thread every 30ms to match DMR timeslot
        //TODO: uncomment after dmr.h merge
        //dmr_main();
        OSTimeDlyHMSM(0u, 0u, 0u, 30u, OS_OPT_TIME_HMSM_STRICT, &os_err);
    }
}

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
    
    // State initialization, execute before starting all tasks
    state_init();

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

    // Create dmr radio thread
    OSTaskCreate((OS_TCB     *) &dmr_tcb,
                 (CPU_CHAR   *) "DMR Task",
                 (OS_TASK_PTR ) dmr_task,
                 (void       *) 0,
                 (OS_PRIO     ) 3,
                 (CPU_STK    *) &dmr_stk[0],
                 (CPU_STK     ) 0,
                 (CPU_STK_SIZE) DMR_TASK_STKSIZE/sizeof(CPU_STK),
                 (OS_MSG_QTY  ) 0,
                 (OS_TICK     ) 0,
                 (void       *) 0,
                 (OS_OPT      ) (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR     *) &os_err);
}
