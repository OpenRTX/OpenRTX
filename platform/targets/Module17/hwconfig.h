/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef HWCONFIG_H
#define HWCONFIG_H

#include "peripherals/i2c.h"
#include "stm32f4xx.h"
#include <stdint.h>
#include <stdbool.h>
#include "pinmap.h"

enum AdcChannel
{
    ADC_HWVER_CH     = 3,
    ADC_HMI_HWVER_CH = 13,
    ADC_VBAT_CH      = 18
};

extern const struct i2cDevice i2c1;
extern const struct i2cDevice i2c2;

enum Mod17HwVersion
{
    MOD17_HW_V01_D = 0,    /* Hardware version 0.1d */
    MOD17_HW_V01_E = 1,    /* Hardware version 0.1e */
    MOD17_HW_V10   = 2     /* Hardware version 1.0  */
};

enum Mod17HmiVersion
{
    MOD17_HMI_V10 = 1     /* HMI hardware ver. 1.0  */
};

enum Mod17Flags
{
    MOD17_FLAGS_HMI_PRESENT = 1,
    MOD17_FLAGS_SOFTPOT     = 2
};

#define MOD17_HWDET_THRESH   300000    /* Threshold for hardware detection, in uV    */

#define MOD17_HW01D_VOLTAGE  0         /* Hardware version 0.1d: gpio pulled to 0V   */
#define MOD17_HW01E_VOLTAGE  3300000   /* Hardware version 0.1e: gpio pulled to 3.3V */
#define MOD17_HW10_VOLTAGE   1650000   /* Hardware version 1.0: gpio pulled to 1.65V */

#define MOD17_HMI10_VOLTAGE  1688000   /* HMI version 1.0: gpio pulled to 1.68V      */


/* Screen dimensions */
#define CONFIG_SCREEN_WIDTH 128
#define CONFIG_SCREEN_HEIGHT 64

/* Screen pixel format */
#define CONFIG_PIX_FMT_BW

/* Device has no battery */
#define CONFIG_BAT_NONE

/* Device supports M17 mode */
#define CONFIG_M17

#endif
