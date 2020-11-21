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

// Allocate state mutex
static OS_MUTEX state_mutex;

// Allocate UI task control block and stack
static OS_TCB  ui_tcb;
static CPU_STK ui_stk[UI_TASK_STKSIZE];

// Allocate state task control block and stack
static OS_TCB  state_tcb;
static CPU_STK state_stk[STATE_TASK_STKSIZE];

// Allocate baseband task control block and stack
static OS_TCB  rtx_tcb;
static CPU_STK rtx_stk[RTX_TASK_STKSIZE];

// Allocate DMR task control block and stack
static OS_TCB  dmr_tcb;
static CPU_STK dmr_stk[DMR_TASK_STKSIZE];


// UI update task
static void ui_task(void *arg)
{
    (void) arg;
    OS_ERR os_err;

    // Initialize keyboard
    kbd_init();
    // Initialize graphics driver
    gfx_init();
    // Initialize user interface
    ui_init();

    // Display splash screen
    ui_drawSplashScreen();
    gfx_render();
    while(gfx_renderingInProgress());
    // Wait 30ms to hide random pixels on screen
    OSTimeDlyHMSM(0u, 0u, 0u, 30u, OS_OPT_TIME_HMSM_STRICT, &os_err);
    platform_setBacklightLevel(255);
    // Keep the splash screen for 1 second
    OSTimeDlyHMSM(0u, 0u, 1u, 0u, OS_OPT_TIME_HMSM_STRICT, &os_err);

    // Get initial state local copy
    // Wait for unlocked mutex and lock it
    OSMutexPend(&state_mutex, 0u, OS_OPT_PEND_BLOCKING, 0u, &os_err);
    state_t last_state = state;
    // Unlock the mutex
    OSMutexPost(&state_mutex, OS_OPT_POST_NONE, &os_err);

    while(1)
    {
        uint32_t keys = kbd_getKeys();
        // Wait for unlocked mutex and lock it
        OSMutexPend(&state_mutex, 0u, OS_OPT_PEND_BLOCKING, 0u, &os_err); 
        // React to keypresses and redraw GUI
        bool renderNeeded = ui_update(last_state, keys);
        // Update state local copy
        last_state = state;
        // Unlock the mutex
        OSMutexPost(&state_mutex, OS_OPT_POST_NONE, &os_err);

        if(renderNeeded)
        {
            gfx_render();
            while(gfx_renderingInProgress());
        }

        // Update UI at ~33 FPS
        OSTimeDlyHMSM(0u, 0u, 0u, 30u, OS_OPT_TIME_HMSM_STRICT, &os_err);
    }
}

// State update task
static void state_task(void *arg)
{
    (void) arg;
    OS_ERR os_err;

    while(1)
    {
        // Wait for unlocked mutex and lock it
        OSMutexPend(&state_mutex, 0u, OS_OPT_PEND_BLOCKING, 0u, &os_err);

        state.time = rtc_getTime(); 
        state.v_bat = platform_getVbat();
        
	    // Unlock the mutex
        OSMutexPost(&state_mutex, OS_OPT_POST_NONE, &os_err);
        
        // Execute state update thread every 1s
        OSTimeDlyHMSM(0u, 0u, 1u, 0u, OS_OPT_TIME_HMSM_STRICT, &os_err);
    }
}

// RTX task
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


// DMR task
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

void create_threads()
{
    OS_ERR os_err;

    // Create state mutex
    OSMutexCreate((OS_MUTEX  *) &state_mutex,
                 (CPU_CHAR   *) "State Mutex",
                 (OS_ERR     *) &os_err);

    // Wait for unlocked mutex and lock it
    OSMutexPend(&state_mutex, 0u, OS_OPT_PEND_BLOCKING, 0u, &os_err);
    // State initialization, execute before starting all tasks
    state_init();
    // Unlock the mutex
    OSMutexPost(&state_mutex, OS_OPT_POST_NONE, &os_err);

    // Create UI thread
    OSTaskCreate((OS_TCB     *) &ui_tcb,
                 (CPU_CHAR   *) "UI Task",
                 (OS_TASK_PTR ) ui_task,
                 (void       *) 0,
                 (OS_PRIO     ) 10,
                 (CPU_STK    *) &ui_stk[0],
                 (CPU_STK     ) 0,
                 (CPU_STK_SIZE) UI_TASK_STKSIZE,
                 (OS_MSG_QTY  ) 0,
                 (OS_TICK     ) 0,
                 (void       *) 0,
                 (OS_OPT      ) (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR     *) &os_err);

    // Create state thread
    OSTaskCreate((OS_TCB     *) &state_tcb,
                 (CPU_CHAR   *) "State Task",
                 (OS_TASK_PTR ) state_task,
                 (void       *) 0,
                 (OS_PRIO     ) 30,
                 (CPU_STK    *) &state_stk[0],
                 (CPU_STK     ) 0,
                 (CPU_STK_SIZE) STATE_TASK_STKSIZE,
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
                 (CPU_STK_SIZE) RTX_TASK_STKSIZE,
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
                 (CPU_STK_SIZE) DMR_TASK_STKSIZE,
                 (OS_MSG_QTY  ) 0,
                 (OS_TICK     ) 0,
                 (void       *) 0,
                 (OS_OPT      ) (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR     *) &os_err);
}
