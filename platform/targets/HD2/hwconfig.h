/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef HWCONFIG_H
#define HWCONFIG_H

#include "registers.h"
#include "peripherals/adc.h"
#include "peripherals/i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Screen: 160x128 ST7735S TFT, RGB565, HW i8080 bus */
#define CONFIG_SCREEN_WIDTH 160
#define CONFIG_SCREEN_HEIGHT 128
#define CONFIG_PIX_FMT_RGB565

/* LCD backlight is a PWM-dimmed LED */
#define CONFIG_SCREEN_BRIGHTNESS

/* On-SoC real-time clock (32.768 kHz xtal, coin-cell backed) */
#define CONFIG_RTC

/* Battery: 2-cell Li-Ion */
#define CONFIG_BAT_LIION
#define CONFIG_BAT_NCELLS 2

/* GPS: module on UART2, polled; NMEA ring-buffer size */
#define CONFIG_GPS
#define CONFIG_NMEA_RBUF_SIZE 512

/* On-chip ADC channels (HR_C7000 ADC block). */
enum AdcChannels {
    ADC_VBAT_CH = 2, /* battery pack, read through a 1:3 divider */
};

/* Board device instances (defined in hwconfig.c).  hwInfo lives in hwconfig.c
 * too; platform.c externs it locally to avoid a hwconfig.h <-> platform.h
 * include cycle (interfaces/platform.h already includes this file). */
extern const struct Adc adc1;
extern const struct i2cDevice i2c1; /* radio bus (AT1846S) */
extern const struct i2cDevice i2c2; /* internal RTC bus */

#ifdef __cplusplus
}
#endif

#endif /* HWCONFIG_H */
