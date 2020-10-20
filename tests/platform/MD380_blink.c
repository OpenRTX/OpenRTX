/***************************************************************************
 *   Copyright (C) 2020 by Federico Izzo IU2NUO, Niccolò Izzo IU2KIN and   *
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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <app_cfg.h>
#include <os.h>
#include <stdio.h>
#include "stm32f4xx.h"
#include "gpio.h"
#include "delays.h"
#include <lib_mem.h>

static OS_TCB        startTCB;
static CPU_STK_SIZE  startStk[APP_CFG_TASK_START_STK_SIZE];
static void startTask(void *arg);

static OS_TCB        t1TCB;
static CPU_STK_SIZE  t1Stk[128];
static void t1(void *arg);

static OS_TCB        t2TCB;
static CPU_STK_SIZE  t2Stk[128];
static void t2(void *arg);

int main(void)
{
    OS_ERR err;

    OSInit(&err);

    OSTaskCreate((OS_TCB     *)&startTCB,
                 (CPU_CHAR   *)" ",
                 (OS_TASK_PTR ) startTask,
                 (void       *) 0,
                 (OS_PRIO     ) APP_CFG_TASK_START_PRIO,
                 (CPU_STK    *)&startStk[0],
                 (CPU_STK     )startStk[APP_CFG_TASK_START_STK_SIZE / 10u],
                 (CPU_STK_SIZE) APP_CFG_TASK_START_STK_SIZE,
                 (OS_MSG_QTY  ) 0,
                 (OS_TICK     ) 0,
                 (void       *) 0,
                 (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR     *)&err);

    OSStart(&err);

    for(;;) ;
    return 0;
}

static void startTask(void* arg)
{

    (void) arg;
    OS_ERR err;

    gpio_setMode(GPIOE, 0, OUTPUT);
    gpio_setMode(GPIOE, 1, OUTPUT);

    CPU_Init();
    OS_CPU_SysTickInitFreq(SystemCoreClock);

    OSTaskCreate((OS_TCB     *)&t1TCB,
                (CPU_CHAR   *)" ",
                (OS_TASK_PTR ) t1,
                (void       *) 0,
                (OS_PRIO     ) 5,
                (CPU_STK    *)&t1Stk[0],
                (CPU_STK     ) 0,
                (CPU_STK_SIZE) 128,
                (OS_MSG_QTY  ) 0,
                (OS_TICK     ) 0,
                (void       *) 0,
                (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                (OS_ERR     *)&err);

    OSTaskCreate((OS_TCB     *)&t2TCB,
                (CPU_CHAR   *)" ",
                (OS_TASK_PTR ) t2,
                (void       *) 0,
                (OS_PRIO     ) 5,
                (CPU_STK    *)&t2Stk[0],
                (CPU_STK     ) 0,
                (CPU_STK_SIZE) 128,
                (OS_MSG_QTY  ) 0,
                (OS_TICK     ) 0,
                (void       *) 0,
                (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                (OS_ERR     *)&err);

    while(1) ;
}

static void t1(void *arg)
{
    (void) arg;
    OS_ERR os_err;

    while(1)
    {
        gpio_togglePin(GPIOE, 0);
        OSTimeDlyHMSM(0u, 0u, 1u, 0u, OS_OPT_TIME_HMSM_STRICT, &os_err);
    }
}

static void t2(void *arg)
{
    (void) arg;
    OS_ERR os_err;

    while(1)
    {
        gpio_togglePin(GPIOE, 1);
        OSTimeDlyHMSM(0u, 0u, 2u, 0u, OS_OPT_TIME_HMSM_STRICT, &os_err);
    }
}
