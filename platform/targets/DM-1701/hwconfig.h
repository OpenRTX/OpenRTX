/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef HWCONFIG_H
#define HWCONFIG_H

#include "stm32f4xx.h"
#include "pinmap.h"

#ifdef __cplusplus

// Export the HR_C6000 driver only for C++ sources
#include "drivers/baseband/HR_C6000.h"

extern HR_C6000 C6000;

extern "C" {
#endif

enum AdcChannel
{
    ADC_VOL_CH   = 0,
    ADC_VBAT_CH  = 1,
    ADC_MIC_CH   = 3,
};

extern const struct spiCustomDevice c6000_spi;
extern const struct spiDevice nvm_spi;
extern const struct Adc adc1;

/* Device has a working real time clock */
#define CONFIG_RTC

/* Screen dimensions */
#define CONFIG_SCREEN_WIDTH 160
#define CONFIG_SCREEN_HEIGHT 128

/* Screen pixel format */
#define CONFIG_PIX_FMT_RGB565

/* Battery type */
#define CONFIG_BAT_LIION
#define CONFIG_BAT_NCELLS 2

/* Device supports M17 mode */
#define CONFIG_M17

/*
 * To enable pwm for display backlight dimming uncomment this directive.
 *
 * WARNING: backlight pwm is disabled by default because it generates a
 * continuous tone in the speaker and headphones.
 *
 * This issue cannot be solved in any way because it derives from how the
 * MD-UV380 mcu pins are used: to have a noiseless backlight pwm, the control
 * pin has to be connected to a mcu pin having between its alternate functions
 * an output compare channel of one of the timers. With this configuration, the
 * pwm signal can completely generated in hardware and its frequency can be well
 * above 22kHz, which is the upper limit for human ears.
 *
 * In the MD-UV380 radio, display backlight is connected to PD8, which is not
 * connected to any of the available output compare channels. Thus, the pwm
 * signal generation is managed inside the TIM11 ISR by toggling the backlight
 * pin and its frequency has to be low (~250Hz) to not put too much overehad on
 * the processor due to timer ISR triggering at an high rate.
 *
 * #define CONFIG_SCREEN_BRIGHTNESS
 */

#ifdef __cplusplus
}
#endif

#endif
