/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef TUSB_CONFIG_H
#define TUSB_CONFIG_H

/*
 * TinyUSB configuration for OpenRTX: one full-speed CDC-ACM device on root
 * hub port 0. The build defines CFG_TUSB_MCU for the MCU it builds for.
 */

#ifndef CFG_TUSB_MCU
#error "CFG_TUSB_MCU must be defined by the build"
#endif

#define CFG_TUSB_OS OPT_OS_NONE
#define CFG_TUSB_DEBUG 0
#define CFG_TUSB_RHPORT0_MODE (OPT_MODE_DEVICE | OPT_MODE_FULL_SPEED)
#define CFG_TUSB_MEM_SECTION
#define CFG_TUSB_MEM_ALIGN __attribute__((aligned(4)))

#define CFG_TUD_ENDPOINT0_SIZE 64

#define CFG_TUD_CDC 1
#define CFG_TUD_MSC 0
#define CFG_TUD_HID 0
#define CFG_TUD_MIDI 0
#define CFG_TUD_VENDOR 0

/*
 * Report carrier (DCD, DSR) to the host on the notification endpoint: the
 * Linux cdc-acm driver otherwise holds open() of the port for seconds.
 */
#define CFG_TUD_CDC_NOTIFY 1

/* FIFOs between the application and the 64-byte bulk endpoints */
#define CFG_TUD_CDC_RX_BUFSIZE 256
#define CFG_TUD_CDC_TX_BUFSIZE 512
#define CFG_TUD_CDC_EP_BUFSIZE 64

#endif /* TUSB_CONFIG_H */
