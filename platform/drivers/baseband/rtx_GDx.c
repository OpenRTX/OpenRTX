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

#include <datatypes.h>
#include <hwconfig.h>
#include <interfaces/platform.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <interfaces/rtx.h>
#include <stdio.h>

OS_MUTEX *cfgMutex;               /* Mutex for incoming config messages */
OS_Q cfgMailbox;                  /* Queue for incoming config messages */

rtxStatus_t rtxStatus;            /* RTX driver status                  */

void rtx_init(OS_MUTEX *m)
{
    /* Initialise mutex for configuration access */
    cfgMutex = m;

    /* Create the message queue for RTX configuration */
    OS_ERR err;
    OSQCreate((OS_Q     *) &cfgMailbox,
              (CPU_CHAR *) "",
              (OS_MSG_QTY) 1,
              (OS_ERR   *) &err);

    /*
     * Default initialisation for rtx status
     */
    rtxStatus.opMode      = FM;
    rtxStatus.bandwidth   = BW_25;
    rtxStatus.txDisable   = 0;
    rtxStatus.opStatus    = OFF;
    rtxStatus.rxFrequency = 430000000;
    rtxStatus.txFrequency = 430000000;
    rtxStatus.txPower     = 0.0f;
    rtxStatus.sqlLevel    = 1;
    rtxStatus.rxTone      = 0;
    rtxStatus.txTone      = 0;
}

void rtx_terminate()
{
}

void rtx_configure(const rtxStatus_t *cfg)
{
    OS_ERR err;
    OSQPost((OS_Q      *) &cfgMailbox,
            (void      *) cfg,
            (OS_MSG_SIZE) sizeof(rtxStatus_t *),
            (OS_OPT     ) OS_OPT_POST_FIFO,
            (OS_ERR    *) &err);

    /*
     * In case message queue is not empty, flush the old and unread configuration
     * and post the new one.
     */
    if(err == OS_ERR_Q_MAX)
    {
        OSQFlush((OS_Q   *) &cfgMailbox,
                 (OS_ERR *) &err);

        OSQPost((OS_Q      *) &cfgMailbox,
                (void      *) cfg,
                (OS_MSG_SIZE) sizeof(rtxStatus_t *),
                (OS_OPT     ) OS_OPT_POST_FIFO,
                (OS_ERR    *) &err);
    }
}

rtxStatus_t rtx_getCurrentStatus()
{
    return rtxStatus;
}

void rtx_taskFunc()
{
    OS_ERR err;
    OS_MSG_SIZE size;
    void *msg = OSQPend((OS_Q        *) &cfgMailbox,
                        (OS_TICK      ) 0,
                        (OS_OPT       ) OS_OPT_PEND_NON_BLOCKING,
                        (OS_MSG_SIZE *) &size,
                        (CPU_TS      *) NULL,
                        (OS_ERR      *) &err);

    if((err == OS_ERR_NONE) && (msg != NULL))
    {
        /* Try locking mutex for read access */
        OSMutexPend(cfgMutex, 0, OS_OPT_PEND_NON_BLOCKING, NULL, &err);

        if(err == OS_ERR_NONE)
        {
            /* Copy new configuration and override opStatus flags */
            uint8_t tmp = rtxStatus.opStatus;
            memcpy(&rtxStatus, msg, sizeof(rtxStatus_t));
            rtxStatus.opStatus = tmp;

            /* Done, release mutex */
            OSMutexPost(cfgMutex, OS_OPT_POST_NONE, &err);
        }

        // Do nothing since this is a stub
    }
}

float rtx_getRssi()
{
    return 0.0;
}
