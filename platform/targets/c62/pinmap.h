/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef PINMAP_H
#define PINMAP_H

/*
 * Pins driven through the gpioPin API. The other pins are assigned in the
 * devicetree, platform/targets/c62/c62.dts.
 */

/* BK4819: its serial clock and data are those of bk4819_spi in c62.dts */
#define BK4819_SCN &GpioA, 8 /* Active low */

/* BK1080 */
#define BK1080_SCK &GpioA, 5
#define BK1080_SDA &GpioA, 6
#define BK1080_PWR &GpioB, 2 /* Active low */

/* BK4819 GPIOs */
#define LNA_VHF_EN &c62_bk4819_gpio, 0
#define LNA_UHF_EN &c62_bk4819_gpio, 1
#define PA_VHF_EN &c62_bk4819_gpio, 2
#define PA_UHF_EN &c62_bk4819_gpio, 3
#define ALC_TX_LED_EN &c62_bk4819_gpio, 4

#endif /* PINMAP_H */
