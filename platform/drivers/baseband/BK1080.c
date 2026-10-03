/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "drivers/baseband/BK1080.h"
#include "interfaces/delays.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))
#endif

// 7-bit I2C-like address of the BK1080, as used by the chip's control byte
// (0x80 = BK1080_I2C_ADDR << 1 | write bit).
#define BK1080_I2C_ADDR 0x40U

static const uint16_t BK1080_RegisterTable[] = {
    0x0008, 0x1080, 0x0201, 0x0000, 0x40C0, 0x0A1F, 0x002E, 0x02FF, 0x5B11,
    0x0000, 0x411E, 0x0000, 0xCE00, 0x0000, 0x0000, 0x1000, 0x3197, 0x0000,
    0x13FF, 0x9852, 0x0000, 0x0000, 0x0008, 0x0000, 0x51E1, 0xA8BC, 0x2645,
    0x00E4, 0x1CD8, 0x3A50, 0xEAE0, 0x3000, 0x0200, 0x0000,
};

static bool gIsInitBK1080;

uint16_t BK1080_BaseFrequency;
uint16_t BK1080_FrequencyDeviation;

enum {
    I2C_WRITE = 0U,
    I2C_READ = 1U,
};

uint16_t BK1080_ReadRegister(const struct BK1080 *dev,
                             BK1080_Register_t Register)
{
    uint8_t ctrl = (uint8_t)((Register << 1) | I2C_READ);
    uint8_t value[2];

    i2c_acquire(dev->i2c);
    i2c_write(dev->i2c, BK1080_I2C_ADDR, &ctrl, sizeof(ctrl), false);
    i2c_read(dev->i2c, BK1080_I2C_ADDR, value, sizeof(value), true);
    i2c_release(dev->i2c);

    return ((uint16_t)value[0] << 8) | value[1];
}

void BK1080_WriteRegister(const struct BK1080 *dev, BK1080_Register_t Register,
                          uint16_t Value)
{
    uint8_t data[3] = { (uint8_t)((Register << 1) | I2C_WRITE),
                        (uint8_t)(Value >> 8), (uint8_t)Value };

    i2c_acquire(dev->i2c);
    i2c_write(dev->i2c, BK1080_I2C_ADDR, data, sizeof(data), true);
    i2c_release(dev->i2c);
}

void BK1080_Init(const struct BK1080 *dev, uint32_t freq, uint8_t band)
{
    unsigned int i;

    gpioPin_setMode(&dev->pwr, OUTPUT);

    if (freq) {
        /* Power pin is active low */
        gpioPin_clear(&dev->pwr);

        if (!gIsInitBK1080) {
            for (i = 0; i < ARRAY_SIZE(BK1080_RegisterTable); i++)
                BK1080_WriteRegister(dev, i, BK1080_RegisterTable[i]);

            delayUs(250000);

            BK1080_WriteRegister(dev, BK1080_REG_25_INTERNAL, 0xA83C);
            BK1080_WriteRegister(dev, BK1080_REG_25_INTERNAL, 0xA8BC);

            delayUs(60000);

            gIsInitBK1080 = true;
        } else {
            BK1080_WriteRegister(dev, BK1080_REG_02_POWER_CONFIGURATION,
                                 0x0201);
        }

        BK1080_WriteRegister(dev, BK1080_REG_05_SYSTEM_CONFIGURATION2, 0x0A1F);
        BK1080_SetFrequency(dev, freq, band);
    } else {
        BK1080_WriteRegister(dev, BK1080_REG_02_POWER_CONFIGURATION, 0x0241);
        gpioPin_set(&dev->pwr);
    }
}

void BK1080_Mute(const struct BK1080 *dev, bool Mute)
{
    BK1080_WriteRegister(dev, BK1080_REG_02_POWER_CONFIGURATION,
                         Mute ? 0x4201 : 0x0201);
}

void BK1080_SetFrequency(const struct BK1080 *dev, uint32_t frequency,
                         uint8_t band)
{
    uint16_t channel = (frequency - BK1080_GetFreqLoLimit(band)) / 100000;

    uint16_t regval = BK1080_ReadRegister(dev,
                                          BK1080_REG_05_SYSTEM_CONFIGURATION2);
    regval = (regval & ~(0b11 << 6)) | ((band & 0b11) << 6);

    BK1080_WriteRegister(dev, BK1080_REG_05_SYSTEM_CONFIGURATION2, regval);

    BK1080_WriteRegister(dev, BK1080_REG_03_CHANNEL, channel);
    delayMs(10);
    BK1080_WriteRegister(dev, BK1080_REG_03_CHANNEL, channel | 0x8000);
}

void BK1080_GetFrequencyDeviation(const struct BK1080 *dev, uint32_t Frequency)
{
    BK1080_BaseFrequency = Frequency;
    BK1080_FrequencyDeviation = BK1080_ReadRegister(dev, BK1080_REG_07) / 16;
}

uint32_t BK1080_GetFreqLoLimit(uint8_t band)
{
    uint32_t lim[] = { 87500000, 76000000, 76000000, 64000000 };
    return lim[band % 4];
}

uint32_t BK1080_GetFreqHiLimit(uint8_t band)
{
    band %= 4;
    uint32_t lim[] = { 108000000, 108000000, 90000000, 76000000 };
    return lim[band % 4];
}
