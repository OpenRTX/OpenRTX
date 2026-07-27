/*
 * SPDX-FileCopyrightText: Copyright 2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Ailunce HD2 board pin map: each GPIO is given a unique functional label
 * (port, pin) so drivers call e.g. gpio_readPin(PTT_SW) instead of a bare
 * gpio_readPin(GPIOB, 11).  Split out of the HR_C7000 SoC register map
 * (mcu/HR_C7000/registers.h) so the SoC header stays board-agnostic.
 */
#ifndef HD2_PINMAP_H
#define HD2_PINMAP_H

#include "registers.h"

/*
 * PTC pad-mux values (SOCSYS IO_DIPLEX2, manual 4.4.4.6 -- upper bits are
 * write-only, so always write the whole-register constant, never
 * read-modify-write):
 *   PTC_GPIO: every PTC pad in GPIO mode (keypad matrix scan).
 *   LCD_I80:  bits 3..18 = 0 -> the HW i8080 controller @0x12000000 owns
 *             PTC3-6 (CS/RS/WR/RD) and PTC7-14 (DB0-7); the rest stays GPIO
 *             (PTC2 = panel reset, ADC/DAC pads).
 * The keypad matrix shares the LCD data pins (rows PTC7-10, cols PTC11-14),
 * so a scan momentarily swaps the mux GPIO<->I80 (see keyboard_HD2.c).
 */
#define HD2_DIPLEX2_PTC_GPIO 0x3ffffffbu
#define HD2_DIPLEX2_LCD_I80 0x3ff80003u

/* Discrete GPIO signals (port, pin) -- live-verified against V2.1.3. */
#define GREEN_LED GPIOB, 0 /* signalling LED, active-high */
#define RED_LED GPIOB, 1   /* signalling LED, active-high */
#define EMER_SW GPIOB, 7   /* emergency side key, active-low */
#define SK1_SW GPIOB, 9    /* side key 1, active-low */
#define PTT_SW GPIOB, 11   /* PTT button, active-low */
#define PWR_SW GPIOB, 12   /* volume/power knob: LOW = on */
#define PWR_HOLD GPIOB, 13 /* power self-latch: HIGH = hold */
#define GPS_PWR GPIOB, 15  /* GPS module power: HIGH = on */
#define ROT_A GPIOA, 0     /* rotary encoder quadrature A */
#define ROT_B GPIOA, 1     /* rotary encoder quadrature B */
#define LCD_RST GPIOC, 2   /* ST7735S panel reset */

/*
 * 4x4 keypad matrix on GPIOC: rows on PTC7-10 (inputs), cols on PTC11-14
 * (driven).  KBD_ROW(n)/KBD_COL(n) expand to the (port, pin) pair for the
 * n-th row/column so the scan loop can label each line.
 */
#define KBD_ROW_SHIFT 7u
#define KBD_COL_SHIFT 11u
#define KBD_ROW_COUNT 4u
#define KBD_COL_COUNT 4u
#define KBD_ROW(n) GPIOC, (KBD_ROW_SHIFT + (n))
#define KBD_COL(n) GPIOC, (KBD_COL_SHIFT + (n))

/* GPIOB analog-FM audio-path pins (driven via the atomic gpio_hrc7000 API, by
 * pin number). Live-verified 2026-06-11. */
#define SPKR_AMP_PIN 4     /* PTB4  speaker-amp mute, active-LOW (LOW = on) */
#define AUDIO_ROUTE_PIN 10 /* PTB10 RX-audio route, LOW = routed */
#define SPKR_GAIN_PIN \
    17 /* PTB17 speaker-amp analog PATH SELECT (NOT a plain gain):
                            * HIGH = AT1846S analog demod (drive HIGH for analog-FM RX
                            * -- the root-cause of the faint-FM-RX saga, LOW = ~inaudible);
                            * LOW  = codec-DAC lineout (MCU beep / voice-prompt playback,
                            * where HIGH would leave the codec DAC silent). */

/* GPIOB analog-FM TX board pins (driven by pin number via gpio_hrc7000). */
#define BAND_SEL_PIN 19 /* PTB19 VHF/UHF band select: HIGH >= 300 MHz */
#define MIC_PATH_PIN 3  /* PTB3  TX mic-path gate: HIGH while keyed */

#endif                  /* HD2_PINMAP_H */
