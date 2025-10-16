/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "interfaces/delays.h"
#include "drivers/baseband/AK2365A.h"

static inline void writeReg(const struct ak2365a *dev, uint8_t reg, uint8_t value)
{
    uint8_t data[2];

    data[0] = (reg & 0x3f) << 1;
    data[1] = value;

    gpioPin_clear(&dev->cs);
    spi_send(dev->spi, &data, 2);
    gpioPin_set(&dev->cs);
}


void AK2365A_init(const struct ak2365a *dev)
{
    gpioPin_setMode(&dev->cs, OUTPUT);
    gpioPin_setMode(&dev->res, OUTPUT);

    gpioPin_clear(&dev->res);
    gpioPin_set(&dev->cs);

    delayUs(100);
    gpioPin_set(&dev->res);

    writeReg(dev, 0x04, 0xAA);   // Software reset
    writeReg(dev, 0x01, 0xC1);   // Operating mode 6, LO freq 50.4MHz
    writeReg(dev, 0x02, 0x33);   // Enable calibration
    writeReg(dev, 0x03, 0x00);   // IF buffer gain 5dB
    writeReg(dev, 0x0B, 0x01);   // AGC auto, AGC1 gain 21dB
    writeReg(dev, 0x0C, 0x80);   // AGC2 gain 12dB
}

void AK2365A_terminate(const struct ak2365a *dev)
{
    gpioPin_clear(&dev->res);
}

void AK2365A_setFilterBandwidth(const struct ak2365a *dev, const uint8_t bw)
{
    uint8_t reg = 0xE1 | (bw << 2);

    writeReg(dev, 0x01, reg);    // Operating mode 7, LO freq. 50.4MHz
    delayMs(1);
    writeReg(dev, 0x01, reg);    // Operating mode 7, LO freq. 50.4MHz
    writeReg(dev, 0x02, 0x1E);   // AGC time = 3, AGC step 2dB
    writeReg(dev, 0x03, 0x00);   // IF buffer gain 5dB
    writeReg(dev, 0x0B, 0x01);   // AGC auto, AGC1 gain 21dB
    writeReg(dev, 0x0C, 0x80);   // AGC2 gain 12dB
}
