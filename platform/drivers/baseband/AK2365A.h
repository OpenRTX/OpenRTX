/***************************************************************************
 *   Copyright (C) 2024 by Silvano Seva IU2KWO                             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#ifndef AK2365A_H
#define AK2365A_H

#include <peripherals/gpio.h>
#include <peripherals/spi.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

enum AK2365A_BPF
{
    AK2365A_BPF_7p5 = 4,    ///< BAND = 1, BPF_BW = 00
    AK2365A_BPF_6   = 0,    ///< BAND = 0, BPF_BW = 00
    AK2365A_BPF_4p5 = 1,    ///< BAND = 0, BPF_BW = 01
    AK2365A_BPF_3   = 2,    ///< BAND = 0, BPF_BW = 10
    AK2365A_BPF_2   = 3,    ///< BAND = 0, BPF_BW = 11
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
