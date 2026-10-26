/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef AK2365A_H
#define AK2365A_H

#include "peripherals/gpio.h"
#include "peripherals/spi.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * IF filter selection, value is BPF_BW[2:0]. BPF_BW[1:0] are bits 3-2 of
 * register 0x01, BPF_BW[2] is bit 7 of register 0x0B. Bandwidths are the
 * -6dB points around the 450kHz IF.
 */
enum AK2365A_BPF
{
    AK2365A_BPF_7p5 = 4,    ///< F0, BPF_BW = 1xx, +/-7.5kHz
    AK2365A_BPF_6   = 0,    ///< F1, BPF_BW = 000, +/-6kHz
    AK2365A_BPF_4p5 = 1,    ///< F2, BPF_BW = 001, +/-4.5kHz
    AK2365A_BPF_3   = 2,    ///< F3, BPF_BW = 010, +/-3kHz
    AK2365A_BPF_2   = 3,    ///< F4, BPF_BW = 011, +/-2kHz
};

/**
 * AK2365A device data.
 */
struct ak2365a
{
    const struct spiDevice *spi;   ///< SPI bus device driver
    const struct gpioPin   cs;     ///< Chip select gpio
    const struct gpioPin   res;    ///< Reset gpio
};


/**
 * Initialise the FM detector IC.
 *
 * @param dev: pointer to device data.
 */
void AK2365A_init(const struct ak2365a *dev);

/**
 * Terminate the driver and set the IC in reset state.
 *
 * @param dev: pointer to device data.
 */
void AK2365A_terminate(const struct ak2365a *dev);

/**
 * Set the bandwidth of the internal IF filter.
 *
 * @param dev: pointer to device data.
 * @param bw: bandwidth.
 */
void AK2365A_setFilterBandwidth(const struct ak2365a *dev, const uint8_t bw);

#ifdef __cplusplus
}
#endif

#endif /* AK2365A_H */
