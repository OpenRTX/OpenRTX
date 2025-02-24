/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef HWCONFIG_H
#define HWCONFIG_H

#include <at32f423.h>
#include "pinmap.h"

/* Screen dimensions */
#define CONFIG_SCREEN_WIDTH 128
#define CONFIG_SCREEN_HEIGHT 64

/* Screen pixel format */
#define CONFIG_PIX_FMT_BW

/* Battery type */
#define CONFIG_BAT_LIION
#define CONFIG_BAT_NCELLS 2

#endif
