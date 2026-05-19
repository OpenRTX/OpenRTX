/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef MAX9814_H
#define MAX9814_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void max9814_setGain(uint8_t gain);

#ifdef __cplusplus
}
#endif

#endif /* MAX9814_H */
