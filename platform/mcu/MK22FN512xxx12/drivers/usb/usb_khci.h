/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
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

#ifndef __USB_KHCI_H__
#define __USB_KHCI_H__

/*******************************************************************************
 * Definitions
 ******************************************************************************/

#define USB_KHCI_BDT_DEVICE_OUT_TOKEN (0x01U)
#define USB_KHCI_BDT_DEVICE_IN_TOKEN (0x09U)
#define USB_KHCI_BDT_DEVICE_SETUP_TOKEN (0x0DU)

#define USB_KHCI_BDT_OWN (0x80U)
#define USB_KHCI_BDT_DATA01(x) ((((uint32_t)(x)) & 0x01U) << 0x06U)
#define USB_KHCI_BDT_BC(x) ((((uint32_t)(x)) & 0x3FFU) << 0x10U)
#define UBS_KHCI_BDT_KEEP (0x20U)
#define UBS_KHCI_BDT_NINC (0x10U)
#define USB_KHCI_BDT_DTS (0x08U)
#define USB_KHCI_BDT_STALL (0x04U)

typedef enum _usb_khci_interrupt_type
{
    kUSB_KhciInterruptReset = 0x01U,
    kUSB_KhciInterruptError = 0x02U,
    kUSB_KhciInterruptSofToken = 0x04U,
    kUSB_KhciInterruptTokenDone = 0x08U,
    kUSB_KhciInterruptSleep = 0x10U,
    kUSB_KhciInterruptResume = 0x20U,
    kUSB_KhciInterruptAttach = 0x40U,
    kUSB_KhciInterruptStall = 0x80U,
} usb_khci_interrupt_type_t;

#endif /* __USB_KHCI_H__ */
