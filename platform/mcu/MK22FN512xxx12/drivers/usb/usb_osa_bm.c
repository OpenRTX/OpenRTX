/*
 * Copyright (c) 2015 - 2016, Freescale Semiconductor, Inc.
 * Copyright 2016 NXP
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "stdint.h"
#include "usb.h"
#include "usb_osa.h"
#include "stdlib.h"
#include "MK22F51212.h"
#include "fsl_common.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define USB_OSA_BM_EVENT_COUNT (2U)
#define USB_OSA_BM_SEM_COUNT (1U)
#define USB_OSA_BM_MSGQ_COUNT (1U)
#define USB_OSA_BM_MSG_COUNT (8U)
#define USB_OSA_BM_MSG_SIZE (4U)

/* BM Event status structure */
typedef struct _usb_osa_event_struct
{
    uint32_t value; /* Event mask */
    uint32_t flag;  /* Event flags, includes auto clear flag */
    uint8_t isUsed; /* Is used */
} usb_osa_event_struct_t;

/* BM semaphore status structure */
typedef struct _usb_osa_sem_struct
{
    uint32_t value; /* Semaphore count */
    uint8_t isUsed; /* Is used */
} usb_osa_sem_struct_t;

/* BM msg status structure */
typedef struct _usb_osa_msg_struct
{
    uint32_t msg[USB_OSA_BM_MSG_SIZE]; /* Message entity pointer */
} usb_osa_msg_struct_t;

/* BM msgq status structure */
typedef struct _usb_osa_msgq_struct
{
    usb_osa_msg_struct_t msgs[USB_OSA_BM_MSG_COUNT]; /* Message entity list */
    uint32_t count;                                  /* Max message entity count */
    uint32_t msgSize;                                /* Size of each message */
    uint32_t msgCount;                               /* Valid messages */
    uint32_t index;                                  /* The first empty message entity index */
    uint32_t current;                                /* The vaild message index */
    uint8_t isUsed;                                  /* Is used */
} usb_osa_msgq_struct_t;

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/

USB_GLOBAL static usb_osa_sem_struct_t s_UsbBmSemStruct[USB_OSA_BM_SEM_COUNT];
USB_GLOBAL static usb_osa_event_struct_t s_UsbBmEventStruct[USB_OSA_BM_EVENT_COUNT];
USB_GLOBAL static usb_osa_msgq_struct_t s_UsbBmMsgqStruct[USB_OSA_BM_MSGQ_COUNT];

/*******************************************************************************
 * Code
 ******************************************************************************/

void USB_OsaEnterCritical(uint32_t *sr)
{
    *sr = __get_PRIMASK();
    __disable_irq();
    __ASM("CPSID I");
}

void USB_OsaExitCritical(uint32_t sr)
{
    __set_PRIMASK(sr);
}
