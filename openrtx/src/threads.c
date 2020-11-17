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

    // Initialise keyboard
    kbd_init();

    // Initialise graphics driver
    gfx_init();
    platform_setBacklightLevel(255);

    // Display splash screen
    point_t splash_origin = {0, SCREEN_HEIGHT / 2};
    color_t yellow_fab413 = {250, 180, 19};
    char *splash_buf = "OpenRTX";
    gfx_clearScreen();
    gfx_print(splash_origin, "OpenRTX", FONT_SIZE_18PT, TEXT_ALIGN_CENTER,
              yellow_fab413);
    gfx_render();
    while(gfx_renderingInProgress());
    OSTimeDlyHMSM(0u, 0u, 1u, 0u, OS_OPT_TIME_HMSM_STRICT, &os_err);

    // Clear screen
    gfx_clearScreen();
    gfx_render();
    while(gfx_renderingInProgress());

    while(1)
    {
        state_t state = state_getCurrentState();
        uint32_t keys = kbd_getKeys();
        bool renderNeeded = ui_update(state, keys);
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

    // Initialise state
    state_init();

    while(1)
    {
        // Execute state thread every 1s
        state_update();
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

    // Create UI thread
    OSTaskCreate((OS_TCB     *) &ui_tcb,
                 (CPU_CHAR   *) " ",
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
                 (CPU_CHAR   *) " ",
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
                 (CPU_CHAR   *) " ",
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
                 (CPU_CHAR   *) " ",
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
