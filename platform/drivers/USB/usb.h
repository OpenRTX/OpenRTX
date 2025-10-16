/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2020-2025 OpenRTX Contributors
 *
 * This file is part of OpenRTX.
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
