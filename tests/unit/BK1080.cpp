/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <vector>
#include "drivers/baseband/BK1080.h"

namespace
{

/**
 * Model of the BK1080 register file behind its serial bus. A frame starts
 * with the device ID, then a control byte with the register address and
 * the direction bit; a write frame carries two data bytes, a read frame is
 * followed by a two byte read.
 */
struct FakeChip {
    uint16_t regs[0x22] = {};
    std::vector<std::pair<uint8_t, uint16_t>> writes;
    int pendingRead = -1;
    bool powered = false;
    int busErrors = 0;
};

FakeChip chip;

int fakeWrite(const struct i2cDevice *, const uint8_t addr, const void *data,
              const size_t len, const bool stop)
{
    const uint8_t *bytes = static_cast<const uint8_t *>(data);

    if (addr != 0x40 || len == 0 || !chip.powered) {
        chip.busErrors++;
        return -1;
    }

    uint8_t reg = bytes[0] >> 1;
    bool read = (bytes[0] & 1) != 0;

    if (read) {
        if (len != 1 || stop)
            chip.busErrors++;
        chip.pendingRead = reg;
        return 0;
    }

    if (len != 3 || !stop) {
        chip.busErrors++;
        return -1;
    }

    uint16_t value = (uint16_t)((bytes[1] << 8) | bytes[2]);
    chip.writes.emplace_back(reg, value);
    /* Device and chip ID are read only */
    if (reg > 0x01 && reg < 0x22)
        chip.regs[reg] = value;
    return 0;
}

int fakeRead(const struct i2cDevice *, const uint8_t, void *data,
             const size_t len, const bool stop)
{
    uint8_t *bytes = static_cast<uint8_t *>(data);

    if (chip.pendingRead < 0 || len != 2 || !stop) {
        chip.busErrors++;
        return -1;
    }

    uint16_t value = chip.regs[chip.pendingRead];
    bytes[0] = value >> 8;
    bytes[1] = value & 0xFF;
    chip.pendingRead = -1;
    return 0;
}

int fakeMode(const struct gpioDev *, const uint8_t, const uint16_t)
{
    return 0;
}

/* The power pin is active low */
void fakeSet(const struct gpioDev *, const uint8_t)
{
    chip.powered = false;
}

void fakeClear(const struct gpioDev *, const uint8_t)
{
    chip.powered = true;
}

bool fakeGet(const struct gpioDev *, const uint8_t)
{
    return !chip.powered;
}

const struct i2cApi fakeI2cApi = { fakeRead, fakeWrite };
const struct i2cDevice fakeI2c = { &fakeI2cApi, nullptr, nullptr };
const struct gpioApi fakeGpioApi = { fakeMode, fakeSet, fakeClear, fakeGet };
const struct gpioDev fakeGpio = { &fakeGpioApi, nullptr };
const struct BK1080 dev = { &fakeI2c, { &fakeGpio, 2 } };

void resetChip()
{
    chip = FakeChip();
    chip.regs[0x00] = 0x0006;
    chip.regs[0x01] = 0x1080;
}

} // namespace

extern "C" void delayUs(unsigned int)
{
}

extern "C" void delayMs(unsigned int)
{
}

TEST_CASE("init powers the chip and loads the vendor table", "[bk1080]")
{
    resetChip();

    REQUIRE(bk1080_init(&dev));
    REQUIRE(chip.powered);
    REQUIRE(chip.busErrors == 0);

    /* Table first, in register order, then the calibration pulse */
    REQUIRE(chip.writes.size() >= 0x22 + 2);
    for (unsigned int i = 0; i < 0x22; i++)
        REQUIRE(chip.writes[i].first == i);
    REQUIRE(chip.writes[0x22]
            == std::make_pair<uint8_t, uint16_t>(0x19, 0xA83C));
    REQUIRE(chip.writes[0x23]
            == std::make_pair<uint8_t, uint16_t>(0x19, 0xA8BC));

    /* 87.5 - 108 MHz, 100 kHz spacing, full volume, enabled and muted */
    REQUIRE(bk1080_getBand(&dev) == BK1080_BAND_87_108);
    REQUIRE((chip.regs[BK1080_REG_SYSCFG2] & BK1080_SYSCFG2_SPACE_MASK)
            == BK1080_SYSCFG2_SPACE_100K);
    REQUIRE((chip.regs[BK1080_REG_SYSCFG2] & BK1080_SYSCFG2_VOLUME_MASK) == 15);
    REQUIRE(chip.regs[BK1080_REG_POWER] == 0x4201);
}

TEST_CASE("init fails when the chip does not answer", "[bk1080]")
{
    resetChip();
    chip.regs[0x01] = 0x0000;

    REQUIRE_FALSE(bk1080_init(&dev));
}

TEST_CASE("terminate disables and powers off the chip", "[bk1080]")
{
    resetChip();
    bk1080_init(&dev);

    bk1080_terminate(&dev);

    REQUIRE(chip.regs[BK1080_REG_POWER] & BK1080_POWER_DISABLE);
    REQUIRE_FALSE(chip.powered);
    REQUIRE(chip.busErrors == 0);
}

TEST_CASE("mute only changes the mute bit", "[bk1080]")
{
    resetChip();
    bk1080_init(&dev);

    bk1080_setMute(&dev, false);
    REQUIRE(chip.regs[BK1080_REG_POWER] == 0x0201);

    bk1080_setMute(&dev, true);
    REQUIRE(chip.regs[BK1080_REG_POWER] == 0x4201);
}

TEST_CASE("frequencies map to channels of the selected band", "[bk1080]")
{
    resetChip();
    bk1080_init(&dev);
    chip.writes.clear();

    REQUIRE(bk1080_setFrequency(&dev, 99500000));

    /* Channel written with TUNE low, then high */
    REQUIRE(chip.writes.size() == 2);
    REQUIRE(chip.writes[0] == std::make_pair<uint8_t, uint16_t>(0x03, 120));
    REQUIRE(chip.writes[1]
            == std::make_pair<uint8_t, uint16_t>(0x03, 120 | 0x8000));

    REQUIRE(bk1080_setFrequency(&dev, 87500000));
    REQUIRE((chip.regs[BK1080_REG_CHANNEL] & BK1080_CHANNEL_MASK) == 0);

    REQUIRE(bk1080_setFrequency(&dev, 108000000));
    REQUIRE((chip.regs[BK1080_REG_CHANNEL] & BK1080_CHANNEL_MASK) == 205);

    /* Rounded down to the channel spacing */
    REQUIRE(bk1080_setFrequency(&dev, 99550000));
    REQUIRE((chip.regs[BK1080_REG_CHANNEL] & BK1080_CHANNEL_MASK) == 120);
}

TEST_CASE("frequencies outside the band are rejected", "[bk1080]")
{
    resetChip();
    bk1080_init(&dev);
    chip.writes.clear();

    REQUIRE_FALSE(bk1080_setFrequency(&dev, 87400000));
    REQUIRE_FALSE(bk1080_setFrequency(&dev, 108100000));
    REQUIRE(chip.writes.empty());

    bk1080_setBand(&dev, BK1080_BAND_76_90);
    REQUIRE(bk1080_getBand(&dev) == BK1080_BAND_76_90);
    REQUIRE(bk1080_setFrequency(&dev, 76000000));
    REQUIRE_FALSE(bk1080_setFrequency(&dev, 99500000));
}

TEST_CASE("band and volume keep the other SYSCFG2 fields", "[bk1080]")
{
    resetChip();
    bk1080_init(&dev);
    uint16_t before = chip.regs[BK1080_REG_SYSCFG2];

    bk1080_setBand(&dev, BK1080_BAND_64_76);
    bk1080_setVolume(&dev, 3);

    uint16_t after = chip.regs[BK1080_REG_SYSCFG2];
    uint16_t others = BK1080_SYSCFG2_SEEKTH_MASK | BK1080_SYSCFG2_SPACE_MASK;
    REQUIRE((after & others) == (before & others));
    REQUIRE((after & BK1080_SYSCFG2_VOLUME_MASK) == 3);
    REQUIRE(bk1080_getBand(&dev) == BK1080_BAND_64_76);

    bk1080_setVolume(&dev, 200);
    REQUIRE((chip.regs[BK1080_REG_SYSCFG2] & BK1080_SYSCFG2_VOLUME_MASK) == 15);
}

TEST_CASE("band limits", "[bk1080]")
{
    REQUIRE(bk1080_bandMinFreq(BK1080_BAND_87_108) == 87500000);
    REQUIRE(bk1080_bandMaxFreq(BK1080_BAND_87_108) == 108000000);
    REQUIRE(bk1080_bandMinFreq(BK1080_BAND_76_108) == 76000000);
    REQUIRE(bk1080_bandMaxFreq(BK1080_BAND_76_90) == 90000000);
    REQUIRE(bk1080_bandMinFreq(BK1080_BAND_64_76) == 64000000);
    REQUIRE(bk1080_bandMaxFreq(BK1080_BAND_64_76) == 76000000);
}

TEST_CASE("status decodes RSSI, SNR, AFC, stereo and offset", "[bk1080]")
{
    resetChip();
    bk1080_init(&dev);
    struct bk1080Status status;

    chip.regs[BK1080_REG_STATUS] = 0x0100 | 42;
    chip.regs[BK1080_REG_FREQ_SNR] = (0x005 << 4) | 9;
    bk1080_getStatus(&dev, &status);
    REQUIRE(status.rssi == 42);
    REQUIRE(status.snr == 9);
    REQUIRE(status.freqOffset == 5);
    REQUIRE(status.stereo);
    REQUIRE_FALSE(status.afcRailed);

    /* Negative offset, AFC at its limit, mono */
    chip.regs[BK1080_REG_STATUS] = 0x1000 | 3;
    chip.regs[BK1080_REG_FREQ_SNR] = (0xFFE << 4) | 0;
    bk1080_getStatus(&dev, &status);
    REQUIRE(status.rssi == 3);
    REQUIRE(status.freqOffset == -2);
    REQUIRE(status.afcRailed);
    REQUIRE_FALSE(status.stereo);
}
