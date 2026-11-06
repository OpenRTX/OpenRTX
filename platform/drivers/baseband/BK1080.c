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

/*
 * Device ID byte of the BK1080. It is sent once at the start of a frame,
 * followed by a control byte holding the register address and the
 * direction bit, see i2c_bitbang.h.
 */
#define BK1080_I2C_ADDR 0x40U

#define CTRL_WRITE 0U
#define CTRL_READ 1U

/*
 * Vendor register values, written to registers 0x00 to 0x21 in order at
 * power on. Registers 0x00 and 0x01 are read only.
 */
static const uint16_t initTable[] = {
    0x0008, 0x1080, 0x0201, 0x0000, 0x40C0, 0x0A1F, 0x002E, 0x02FF, 0x5B11,
    0x0000, 0x411E, 0x0000, 0xCE00, 0x0000, 0x0000, 0x1000, 0x3197, 0x0000,
    0x13FF, 0x9852, 0x0000, 0x0000, 0x0008, 0x0000, 0x51E1, 0xA8BC, 0x2645,
    0x00E4, 0x1CD8, 0x3A50, 0xEAE0, 0x3000, 0x0200, 0x0000,
};

/*
 * Vendor calibration sequence: register 25 is pulsed with bit 7 low after
 * the table settles.
 */
#define INTERNAL_25_CALIBRATE 0xA83CU
#define INTERNAL_25_NORMAL 0xA8BCU

#define INIT_SETTLE_US 250000U
#define CALIBRATE_SETTLE_US 60000U
#define TUNE_DELAY_MS 10U

static const uint32_t bandMin[] = { 87500000, 76000000, 76000000, 64000000 };
static const uint32_t bandMax[] = { 108000000, 108000000, 90000000, 76000000 };

/**
 * Change some bits of a register.
 *
 * @param dev: pointer to device data.
 * @param reg: register address.
 * @param mask: bits to change.
 * @param value: new value of those bits.
 */
static void updateReg(const struct BK1080 *dev, enum bk1080Reg reg,
                      uint16_t mask, uint16_t value)
{
    uint16_t regval = bk1080_readReg(dev, reg);

    regval = (regval & ~mask) | (value & mask);
    bk1080_writeReg(dev, reg, regval);
}

uint16_t bk1080_readReg(const struct BK1080 *dev, enum bk1080Reg reg)
{
    uint8_t ctrl = (uint8_t)((reg << 1) | CTRL_READ);
    uint8_t value[2] = { 0 };

    i2c_acquire(dev->i2c);
    i2c_write(dev->i2c, BK1080_I2C_ADDR, &ctrl, sizeof(ctrl), false);
    i2c_read(dev->i2c, BK1080_I2C_ADDR, value, sizeof(value), true);
    i2c_release(dev->i2c);

    return ((uint16_t)value[0] << 8) | value[1];
}

void bk1080_writeReg(const struct BK1080 *dev, enum bk1080Reg reg,
                     uint16_t value)
{
    uint8_t data[3] = { (uint8_t)((reg << 1) | CTRL_WRITE),
                        (uint8_t)(value >> 8), (uint8_t)value };

    i2c_acquire(dev->i2c);
    i2c_write(dev->i2c, BK1080_I2C_ADDR, data, sizeof(data), true);
    i2c_release(dev->i2c);
}

bool bk1080_init(const struct BK1080 *dev)
{
    gpioPin_setMode(&dev->pwr, OUTPUT);
    gpioPin_clear(&dev->pwr);

    for (unsigned int i = 0; i < ARRAY_SIZE(initTable); i++)
        bk1080_writeReg(dev, (enum bk1080Reg)i, initTable[i]);

    delayUs(INIT_SETTLE_US);

    bk1080_writeReg(dev, BK1080_REG_INTERNAL_25, INTERNAL_25_CALIBRATE);
    bk1080_writeReg(dev, BK1080_REG_INTERNAL_25, INTERNAL_25_NORMAL);

    delayUs(CALIBRATE_SETTLE_US);

    bk1080_setMute(dev, true);

    return bk1080_readReg(dev, BK1080_REG_CHIP_ID) == BK1080_CHIP_ID;
}

void bk1080_terminate(const struct BK1080 *dev)
{
    updateReg(dev, BK1080_REG_POWER, BK1080_POWER_DISABLE,
              BK1080_POWER_DISABLE);
    gpioPin_set(&dev->pwr);
}

uint32_t bk1080_bandMinFreq(enum bk1080Band band)
{
    return bandMin[band & 0x3];
}

uint32_t bk1080_bandMaxFreq(enum bk1080Band band)
{
    return bandMax[band & 0x3];
}

void bk1080_setBand(const struct BK1080 *dev, enum bk1080Band band)
{
    updateReg(dev, BK1080_REG_SYSCFG2, BK1080_SYSCFG2_BAND_MASK,
              (uint16_t)band << BK1080_SYSCFG2_BAND_SHIFT);
}

enum bk1080Band bk1080_getBand(const struct BK1080 *dev)
{
    uint16_t regval = bk1080_readReg(dev, BK1080_REG_SYSCFG2);

    return (enum bk1080Band)((regval & BK1080_SYSCFG2_BAND_MASK)
                             >> BK1080_SYSCFG2_BAND_SHIFT);
}

bool bk1080_setFrequency(const struct BK1080 *dev, uint32_t freq)
{
    enum bk1080Band band = bk1080_getBand(dev);

    if (freq < bk1080_bandMinFreq(band) || freq > bk1080_bandMaxFreq(band))
        return false;

    uint16_t channel = (freq - bk1080_bandMinFreq(band))
                     / BK1080_CHANNEL_SPACING;

    /* The chip starts tuning on a rising edge of the TUNE bit */
    bk1080_writeReg(dev, BK1080_REG_CHANNEL, channel);
    delayMs(TUNE_DELAY_MS);
    bk1080_writeReg(dev, BK1080_REG_CHANNEL, channel | BK1080_CHANNEL_TUNE);

    return true;
}

void bk1080_setVolume(const struct BK1080 *dev, uint8_t volume)
{
    if (volume > BK1080_VOLUME_MAX)
        volume = BK1080_VOLUME_MAX;

    updateReg(dev, BK1080_REG_SYSCFG2, BK1080_SYSCFG2_VOLUME_MASK, volume);
}

void bk1080_setMute(const struct BK1080 *dev, bool mute)
{
    updateReg(dev, BK1080_REG_POWER, BK1080_POWER_MUTE,
              mute ? BK1080_POWER_MUTE : 0);
}

void bk1080_getStatus(const struct BK1080 *dev, struct bk1080Status *status)
{
    uint16_t statusReg = bk1080_readReg(dev, BK1080_REG_STATUS);
    uint16_t freqSnr = bk1080_readReg(dev, BK1080_REG_FREQ_SNR);

    /* FREQD is a 12-bit two's complement value */
    int16_t freqd = (int16_t)((freqSnr & BK1080_FREQ_SNR_FREQD_MASK)
                              >> BK1080_FREQ_SNR_FREQD_SHIFT);
    if (freqd & 0x800)
        freqd -= 0x1000;

    status->rssi = statusReg & BK1080_STATUS_RSSI_MASK;
    status->snr = freqSnr & BK1080_FREQ_SNR_SNR_MASK;
    status->freqOffset = freqd;
    status->afcRailed = (statusReg & BK1080_STATUS_AFC_RAILED) != 0;
    status->stereo = (statusReg & BK1080_STATUS_STEREO) != 0;
}
