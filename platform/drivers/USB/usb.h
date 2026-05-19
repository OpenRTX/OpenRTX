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
 * Initialise the USB driver.
 */
void usb_init();

/**
 * Shut down the USB driver.
 */
void usb_terminate();

#ifdef __cplusplus
}
#endif

#endif /* USB_H */
