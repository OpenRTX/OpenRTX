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

#include <stdlib.h>
#include <app_cfg.h>
#include <os.h>
#include "delays.h"
#include "state.h"
#include "ui.h"

// Allocate UI thread task control block and stack
static OS_TCB       ui_tcb;
static CPU_STK_SIZE ui_stk[128];
static void ui_task(void *arg);

// Allocate state thread task control block and stack
static OS_TCB       state_tcb;
static CPU_STK_SIZE state_stk[128];
static void state_task(void *arg);

// Allocate rtx radio thread task control block and stack
static OS_TCB       rtx_tcb;
static CPU_STK_SIZE rtx_stk[128];
static void rtx_task(void *arg);

// Allocate dmr radio thread task control block and stack
static OS_TCB       dmr_tcb;
static CPU_STK_SIZE dmr_stk[128];
static void dmr_task(void *arg);


void create_threads()
{
    OS_ERR os_err;

    // Create UI thread
    OSTaskCreate((OS_TCB     *)&ui_tcb,
                 (CPU_CHAR   *)" ",
                 (OS_TASK_PTR ) ui_task,
                 (void       *) 0,
                 (OS_PRIO     ) 5,
                 (CPU_STK    *)&ui_stk[0],
                 (CPU_STK     ) 0,
                 (CPU_STK_SIZE) 128,
                 (OS_MSG_QTY  ) 0,
                 (OS_TICK     ) 0,
                 (void       *) 0,
                 (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR     *)&os_err);

    // Create state thread
    OSTaskCreate((OS_TCB     *)&state_tcb,
                 (CPU_CHAR   *)" ",
                 (OS_TASK_PTR ) state_task,
                 (void       *) 0,
                 (OS_PRIO     ) 5,
                 (CPU_STK    *)&state_stk[0],
                 (CPU_STK     ) 0,
                 (CPU_STK_SIZE) 128,
                 (OS_MSG_QTY  ) 0,
                 (OS_TICK     ) 0,
                 (void       *) 0,
                 (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR     *)&os_err);
    
    // Create rtx radio thread
    OSTaskCreate((OS_TCB     *)&rtx_tcb,
                 (CPU_CHAR   *)" ",
                 (OS_TASK_PTR ) rtx_task,
                 (void       *) 0,
                 (OS_PRIO     ) 5,
                 (CPU_STK    *)&rtx_stk[0],
                 (CPU_STK     ) 0,
                 (CPU_STK_SIZE) 128,
                 (OS_MSG_QTY  ) 0,
                 (OS_TICK     ) 0,
                 (void       *) 0,
                 (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR     *)&os_err);
    
    // Create dmr radio thread
    OSTaskCreate((OS_TCB     *)&dmr_tcb,
                 (CPU_CHAR   *)" ",
                 (OS_TASK_PTR ) dmr_task,
                 (void       *) 0,
                 (OS_PRIO     ) 5,
                 (CPU_STK    *)&dmr_stk[0],
                 (CPU_STK     ) 0,
                 (CPU_STK_SIZE) 128,
                 (OS_MSG_QTY  ) 0,
                 (OS_TICK     ) 0,
                 (void       *) 0,
                 (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR     *)&os_err);
}

int main(void)
{
    // Initialize the radio state
    state_init();
    
    // Create OpenRTX threads
    create_threads();
    
    while(1) ;
    return 0;
}

static void ui_task(void *arg)
{
    (void) arg;
    OS_ERR os_err; 
    while(1)
    {
        // Execute UI thread every 33ms to get ~30FPS
        ui_main();
        OSTimeDlyHMSM(0u, 0u, 0u, 33u, OS_OPT_TIME_HMSM_STRICT, &os_err);
    }
}

static void state_task(void *arg)
{
    (void) arg;
    OS_ERR os_err; 
    while(1)
    {
        // Execute state thread every 1s
        state_main();
        OSTimeDlyHMSM(0u, 0u, 1u, 0u, OS_OPT_TIME_HMSM_STRICT, &os_err);
    }
}

static void rtx_task(void *arg)
{
    (void) arg;
    OS_ERR os_err; 
    while(1)
    {
        // Execute rtx radio thread every 33ms to match UI refresh
        //TODO: uncomment after rtx.h merge
        //rtx_main();
        OSTimeDlyHMSM(0u, 0u, 0u, 33u, OS_OPT_TIME_HMSM_STRICT, &os_err);
    }
}

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
