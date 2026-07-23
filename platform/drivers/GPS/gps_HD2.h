/*
 * SPDX-FileCopyrightText: Copyright 2026 OpenRTX Contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * GPS driver for the Ailunce HD2 (HR_C7000 / CK803S): the GPS module is on
 * UART2, polled, feeding OpenRTX's NMEA ring buffer.
 */

#ifndef GPS_HD2_H
#define GPS_HD2_H

#include "core/gps.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Bring up UART2 + power the GPS module and return the OpenRTX gpsDevice
 * handle (call once, from platform_initGps()). */
const struct gpsDevice *gps_HD2_init(void);

#ifdef __cplusplus
}
#endif

#endif /* GPS_HD2_H */
