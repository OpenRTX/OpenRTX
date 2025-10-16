/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "interfaces/delays.h"
#include <math.h>
#include "drivers/baseband/SKY72310.h"

static inline void writeReg(const struct sky73210 *dev, const uint16_t value)
{
    const uint16_t tmp = __builtin_bswap16(value);

    /*
     * NOTE: for some (yet) unknown reason, there must be at least 10us between
     * chip select assertion/deassertion and the SPI transaction. For lower
     * values the PLL seems to go nuts.
     */
    spi_acquire(dev->spi);
    gpioPin_clear(&dev->cs);
    delayUs(10);

    spi_send(dev->spi, &tmp, 2);

    delayUs(10);
    gpioPin_set(&dev->cs);
    spi_release(dev->spi);
}


void SKY73210_init(const struct sky73210 *dev)
{
    gpioPin_setMode(&dev->cs, OUTPUT);
    gpioPin_set(&dev->cs);

    writeReg(dev, 0x6000 | 0x1F); // Phase detector gain
    writeReg(dev, 0x73D0);        // Power down/multiplexer control register
    writeReg(dev, 0x8000);        // Modulation control register
    writeReg(dev, 0x9000);        // Modulation data register
}

void SKY73210_terminate(const struct sky73210 *dev)
{
    (void) dev;
}

void SKY73210_setFrequency(const struct sky73210 *dev, const uint32_t freq,
                           uint8_t clkDiv)
{
    // Maximum allowable value for reference clock divider is 32
    if (clkDiv > 32)
        clkDiv = 32;

    float clk = ((float) dev->refClk) / ((float) clkDiv);
    float K = ((float) freq) / clk;
    float Ndiv = floor(K) - 32.0;
    float Ndnd = round(262144 * (K - Ndiv - 32.0));

    /*
     * With PLL in fractional mode, dividend range is [-131017 +131071].
     * When converting from float to uint32_t we have to cast the value to a
     * signed 18-bit one and increment the divider by one if dividend is negative.
     */
    uint32_t dnd = ((uint32_t) Ndnd) & 0x03FFFF;
    if(dnd & 0x20000)
        Ndiv += 1;

    uint16_t dndMsb = dnd >> 8;
    uint16_t dndLsb = dnd & 0x00FF;

    writeReg(dev, (uint16_t) Ndiv);                   // Divider register
    writeReg(dev, 0x2000 | dndLsb);                   // Dividend LSB register
    writeReg(dev, 0x1000 | dndMsb);                   // Dividend MSB register
    writeReg(dev, 0x5000 | ((uint16_t)clkDiv - 1));   // Reference clock divider
}
