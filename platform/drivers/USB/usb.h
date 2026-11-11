/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef USB_H
#define USB_H

#include <stdint.h>
#include <unistd.h>
#include "tusb.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Bring up the MCU's USB device controller and start TinyUSB on it. Each MCU
 * that runs TinyUSB implements this; vcom_tinyusb.c calls it.
 *
 * @return 0 on success, a negative error code when the controller does not
 * start.
 */
int usb_init();

/**
 * Shut down the USB driver.
 */
void usb_terminate();

#ifdef __cplusplus
}
#endif

#endif /* USB_H */
