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
 *   As a special exception, if other files instantiate templates or use   *
 *   macros or inline functions from this file, or you compile this file   *
 *   and link it with other works to produce a work based on this file,    *
 *   this file does not by itself cause the resulting work to be covered   *
 *   by the GNU General Public License. However the source code for this   *
 *   file must still be made available in accordance with the GNU General  *
 *   Public License. This exception does not invalidate any other reasons  *
 *   why a work based on this file might be covered by the GNU General     *
 *   Public License.                                                       *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#include <stdio.h>
#include <app_cfg.h>
#include <os.h>
#include <hwconfig.h>

/*
 * Entry point for application code, not in this translation unit.
 */
int main(int argc, char *argv[]);

/*
 * OS startup task, will call main() when all initialisations are done.
 */
#define START_TSK_STKSIZE 2048/sizeof(CPU_STK)
static OS_TCB  startTCB;
static CPU_STK startStk[START_TSK_STKSIZE];
static void startTask(void *arg);

void systemBootstrap()
{
    OS_ERR err;

    OSInit(&err);

    OSTaskCreate((OS_TCB     *) &startTCB,
                 (CPU_CHAR   *) " ",
                 (OS_TASK_PTR ) startTask,
                 (void       *) 0,
                 (OS_PRIO     ) APP_CFG_TASK_START_PRIO,
                 (CPU_STK    *) &startStk[0],
                 (CPU_STK     ) startStk[START_TSK_STKSIZE / 10u],
                 (CPU_STK_SIZE) START_TSK_STKSIZE,
                 (OS_MSG_QTY  ) 0,
                 (OS_TICK     ) 0,
                 (void       *) 0,
                 (OS_OPT      ) (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR     *) &err);

    OSStart(&err);

    for(;;) ;
}

static void startTask(void* arg)
{
    (void) arg;

    CPU_Init();

    /* On ARM-based targets setup SysTick */
    #ifdef __arm__
    OS_CPU_SysTickInitFreq(SystemCoreClock);
    #else
    OS_CPU_SysTickInit();
    #endif

    /* Jump to application code */
    main(0, NULL);

    /* If main returns loop indefinitely */
    for(;;) ;
}
