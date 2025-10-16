/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef HWCONFIG_H
#define HWCONFIG_H

#include "MK22F51212.h"

#ifdef PLATFORM_GD77
#include "pinmap_GD77.h"
#else
#include "pinmap_DM1801.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern const struct spiCustomDevice nvm_spi;
extern const struct spiDevice c6000_spi;

/* Screen dimensions */
#define CONFIG_SCREEN_WIDTH 128
#define CONFIG_SCREEN_HEIGHT 64

/* Screen pixel format */
#define CONFIG_PIX_FMT_BW

/* Screen has adjustable contrast */
#define CONFIG_SCREEN_CONTRAST
#define CONFIG_DEFAULT_CONTRAST 71

/* Screen has adjustable brightness */
#define CONFIG_SCREEN_BRIGHTNESS

/* Battery type */
#define CONFIG_BAT_LIION
#define CONFIG_BAT_NCELLS 2

#ifdef __cplusplus
}
#endif

#endif /* HWCONFIG_H */
