/*
 * SPDX-FileCopyrightText: Copyright 2026 HD2 Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef GPS_HD2_H
#define GPS_HD2_H

#include "core/gps.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialise the HD2 GPS driver (UART2 @ 0x14050000, 9600 8N1) and
 * return the gpsDevice handle for the OpenRTX GPS stack.
 *
 * @return pointer to the (static) HD2 gpsDevice.
 */
const struct gpsDevice *gps_HD2_init(void);

#ifdef __cplusplus
}
#endif

#endif /* GPS_HD2_H */
