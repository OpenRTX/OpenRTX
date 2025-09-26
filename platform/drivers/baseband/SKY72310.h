/***************************************************************************
 *   Copyright (C) 2020 - 2025 by Silvano Seva IU2KWO                      *
 *                            and Niccolò Izzo IU2KIN                      *
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

#ifndef SKY73210_H
#define SKY73210_H

#include "peripherals/gpio.h"
#include "peripherals/spi.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * SKY73210 device data.
 */
struct sky73210
{
    const struct spiDevice *spi;      ///< SPI bus device driver
    const struct gpioPin   cs;        ///< Chip select gpio
    const uint32_t         refClk;    ///< Reference clock frequency, in Hz
};

/**
 * Initialise the PLL.
 *
 * @param dev: pointer to device data.
 */
void SKY73210_init(const struct sky73210 *dev);

/**
 * Terminate PLL driver.
 *
 * @param dev: pointer to device data.
 */
void SKY73210_terminate(const struct sky73210 *dev);

/**
 * Change VCO frequency.
 *
 * @param dev: pointer to device data.
 * @param freq: new VCO frequency, in Hz.
 * @param clkDiv: reference clock division factor.
 */
void SKY73210_setFrequency(const struct sky73210 *dev, const uint32_t freq,
                           uint8_t clkDiv);

#ifdef __cplusplus
}
#endif

#endif /* SKY73210_H */
