/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "protocols/APRS/constants.h"

#ifndef APRS_FRAME_H
#define APRS_FRAME_H

#ifdef __cplusplus
extern "C" {
#endif

struct frameData {
    uint8_t data[APRS_PACLEN];
    uint8_t len;
};

#ifdef __cplusplus
}
#endif

#endif /* APRS_FRAME_H */
