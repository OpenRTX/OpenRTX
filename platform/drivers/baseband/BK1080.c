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

static void I2C_Start(const struct BK1080 *dev);
static void I2C_Stop(const struct BK1080 *dev);

static uint8_t I2C_Read(const struct BK1080 *dev, bool bFinal);
static int I2C_Write(const struct BK1080 *dev, uint8_t Data);

static int I2C_ReadBuffer(const struct BK1080 *dev, void *pBuffer,
                          uint8_t Size);
static int I2C_WriteBuffer(const struct BK1080 *dev, const void *pBuffer,
                           uint8_t Size);

static void I2C_Start(const struct BK1080 *dev)
{
    gpioPin_set(&dev->sda);
    delayUs(1);
    gpioPin_set(&dev->sck);
    delayUs(1);
    gpioPin_clear(&dev->sda);
    delayUs(1);
    gpioPin_clear(&dev->sck);
    delayUs(1);
}

static void I2C_Stop(const struct BK1080 *dev)
{
    gpioPin_clear(&dev->sda);
    delayUs(1);
    gpioPin_clear(&dev->sck);
    delayUs(1);
    gpioPin_set(&dev->sck);
    delayUs(1);
    gpioPin_set(&dev->sda);
    delayUs(1);
}

static uint8_t I2C_Read(const struct BK1080 *dev, bool bFinal)
{
    uint8_t i, Data;

    gpioPin_setMode(&dev->sda, INPUT_PULL_UP);

    Data = 0;
    for (i = 0; i < 8; i++) {
        gpioPin_clear(&dev->sck);
        delayUs(1);
        gpioPin_set(&dev->sck);
        delayUs(1);
        Data <<= 1;
        delayUs(1);
        if (gpioPin_read(&dev->sda)) {
            Data |= 1U;
        }
        gpioPin_clear(&dev->sck);
        delayUs(1);
    }

    gpioPin_setMode(&dev->sda, OUTPUT);
    gpioPin_clear(&dev->sck);
    delayUs(1);
    if (bFinal) {
        gpioPin_set(&dev->sda);
    } else {
        gpioPin_clear(&dev->sda);
    }
    delayUs(1);
    gpioPin_set(&dev->sck);
    delayUs(1);
    gpioPin_clear(&dev->sck);
    delayUs(1);

    return Data;
}

static int I2C_Write(const struct BK1080 *dev, uint8_t Data)
{
    uint8_t i;
    int ret = -1;

    gpioPin_clear(&dev->sck);
    delayUs(1);
    for (i = 0; i < 8; i++) {
        if ((Data & 0x80) == 0) {
            gpioPin_clear(&dev->sda);
        } else {
            gpioPin_set(&dev->sda);
        }
        Data <<= 1;
        delayUs(1);
        gpioPin_set(&dev->sck);
        delayUs(1);
        gpioPin_clear(&dev->sck);
        delayUs(1);
    }

    gpioPin_setMode(&dev->sda, INPUT_PULL_UP);
    gpioPin_set(&dev->sda);
    delayUs(1);
    gpioPin_set(&dev->sck);
    delayUs(1);

    for (i = 0; i < 255; i++) {
        if (gpioPin_read(&dev->sda) == 0) {
            ret = 0;
            break;
        }
    }

    gpioPin_clear(&dev->sck);
    delayUs(1);
    gpioPin_setMode(&dev->sda, OUTPUT);
    gpioPin_clear(&dev->sda);

    return ret;
}

static int I2C_ReadBuffer(const struct BK1080 *dev, void *pBuffer, uint8_t Size)
{
    uint8_t *pData = (uint8_t *)pBuffer;
    uint8_t i;

    for (i = 0; i < Size - 1; i++) {
        delayUs(1);
        pData[i] = I2C_Read(dev, false);
    }

    delayUs(1);
    pData[i] = I2C_Read(dev, true);

    return Size;
}

static int I2C_WriteBuffer(const struct BK1080 *dev, const void *pBuffer,
                           uint8_t Size)
{
    const uint8_t *pData = (const uint8_t *)pBuffer;
    uint8_t i;

    for (i = 0; i < Size; i++) {
        if (I2C_Write(dev, *pData++) < 0) {
            return -1;
        }
    }

    return 0;
}

uint16_t BK1080_ReadRegister(const struct BK1080 *dev,
                             BK1080_Register_t Register)
{
    uint8_t Value[2];

    I2C_Start(dev);
    I2C_Write(dev, 0x80);
    I2C_Write(dev, (Register << 1) | I2C_READ);
    I2C_ReadBuffer(dev, Value, sizeof(Value));
    I2C_Stop(dev);

    return (Value[0] << 8) | Value[1];
}

void BK1080_WriteRegister(const struct BK1080 *dev, BK1080_Register_t Register,
                          uint16_t Value)
{
    I2C_Start(dev);
    I2C_Write(dev, 0x80);
    I2C_Write(dev, (Register << 1) | I2C_WRITE);
    Value = ((Value >> 8) & 0xFF) | ((Value & 0xFF) << 8);
    I2C_WriteBuffer(dev, &Value, sizeof(Value));
    I2C_Stop(dev);
}

void BK1080_Init(const struct BK1080 *dev, uint32_t freq, uint8_t band)
{
    unsigned int i;

    gpioPin_setMode(&dev->sck, OUTPUT);
    gpioPin_setMode(&dev->sda, OUTPUT);
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
