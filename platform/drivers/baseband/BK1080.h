/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef BK1080_H
#define BK1080_H

#include <stdint.h>
#include <stdbool.h>
#include "peripherals/gpio.h"
#include "peripherals/i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Driver for the Beken BK1080 broadcast FM receiver.
 *
 * The register map follows the Si470x layout. Tuning, band, volume, mute,
 * RSSI, SNR, frequency offset and AFC have been checked on a Retevis C62;
 * the stereo indicator has not.
 *
 * Typical use:
 *
 *   bk1080_init(&dev);
 *   bk1080_setBand(&dev, BK1080_BAND_87_108);
 *   bk1080_setFrequency(&dev, 99500000);
 *   bk1080_setMute(&dev, false);
 *   ...
 *   bk1080_terminate(&dev);
 */

/**
 * Register addresses.
 */
enum bk1080Reg {
    BK1080_REG_DEVICE_ID = 0x00,
    BK1080_REG_CHIP_ID = 0x01,
    BK1080_REG_POWER = 0x02,
    BK1080_REG_CHANNEL = 0x03,
    BK1080_REG_SYSCFG1 = 0x04,
    BK1080_REG_SYSCFG2 = 0x05,
    BK1080_REG_SYSCFG3 = 0x06,
    BK1080_REG_FREQ_SNR = 0x07,
    BK1080_REG_STATUS = 0x0A,
    BK1080_REG_READ_CHANNEL = 0x0B,
    BK1080_REG_INTERNAL_25 = 0x19,
};

/* Value of BK1080_REG_CHIP_ID */
#define BK1080_CHIP_ID 0x1080U

/* BK1080_REG_POWER */
#define BK1080_POWER_MUTE (1U << 14)
#define BK1080_POWER_MONO (1U << 13)
#define BK1080_POWER_DISABLE (1U << 6)
#define BK1080_POWER_ENABLE (1U << 0)

/* BK1080_REG_CHANNEL */
#define BK1080_CHANNEL_TUNE (1U << 15)
#define BK1080_CHANNEL_MASK 0x03FFU

/* BK1080_REG_SYSCFG2 */
#define BK1080_SYSCFG2_SEEKTH_SHIFT 8
#define BK1080_SYSCFG2_SEEKTH_MASK (0xFFU << BK1080_SYSCFG2_SEEKTH_SHIFT)
#define BK1080_SYSCFG2_BAND_SHIFT 6
#define BK1080_SYSCFG2_BAND_MASK (0x3U << BK1080_SYSCFG2_BAND_SHIFT)
#define BK1080_SYSCFG2_SPACE_SHIFT 4
#define BK1080_SYSCFG2_SPACE_MASK (0x3U << BK1080_SYSCFG2_SPACE_SHIFT)
#define BK1080_SYSCFG2_SPACE_100K (0x1U << BK1080_SYSCFG2_SPACE_SHIFT)
#define BK1080_SYSCFG2_VOLUME_MASK 0x000FU

/* BK1080_REG_FREQ_SNR */
#define BK1080_FREQ_SNR_FREQD_SHIFT 4
#define BK1080_FREQ_SNR_FREQD_MASK (0xFFFU << BK1080_FREQ_SNR_FREQD_SHIFT)
#define BK1080_FREQ_SNR_SNR_MASK 0x000FU

/*
 * BK1080_REG_STATUS. STEREO is not used by the vendor code and read 0 on
 * a stereo station at SNR 8-10, so its position is unconfirmed.
 */
#define BK1080_STATUS_AFC_RAILED (1U << 12)
#define BK1080_STATUS_STEREO (1U << 8)
#define BK1080_STATUS_RSSI_MASK 0x00FFU

/** Channel spacing set by bk1080_init(), in Hz. */
#define BK1080_CHANNEL_SPACING 100000U

/** Highest volume step accepted by bk1080_setVolume(). */
#define BK1080_VOLUME_MAX 15U

/**
 * Frequency bands, as encoded in BK1080_REG_SYSCFG2.
 */
enum bk1080Band {
    BK1080_BAND_87_108 = 0, ///< 87.5 - 108 MHz, Europe and the Americas
    BK1080_BAND_76_108 = 1, ///< 76 - 108 MHz, worldwide
    BK1080_BAND_76_90 = 2,  ///< 76 - 90 MHz, Japan
    BK1080_BAND_64_76 = 3,  ///< 64 - 76 MHz, Eastern Europe
};

/**
 * BK1080 device data.
 *
 * Register access happens over the chip's I2C-like serial bus, accessed
 * through the generic struct i2cDevice interface of peripherals/i2c.h.
 */
struct BK1080 {
    const struct i2cDevice *i2c; ///< I2C bus device
    struct gpioPin pwr;          ///< Power switch, active low
};

/**
 * Reception quality, as reported by the chip for the tuned frequency.
 */
struct bk1080Status {
    uint8_t rssi;       ///< Received signal strength, in dBuV
    uint8_t snr;        ///< Signal to noise ratio, 0 to 15
    int16_t freqOffset; ///< Frequency offset, in chip units, signed
    bool afcRailed;     ///< AFC at its limit, no station on frequency
    bool stereo;        ///< Stereo pilot detected
};

/**
 * Power on the chip and load its default configuration: 87.5 - 108 MHz
 * band, 100 kHz channel spacing, maximum volume, audio muted.
 *
 * @param dev: pointer to device data.
 * @return true if the chip answered with its chip ID.
 */
bool bk1080_init(const struct BK1080 *dev);

/**
 * Power off the chip.
 *
 * @param dev: pointer to device data.
 */
void bk1080_terminate(const struct BK1080 *dev);

/**
 * Read a chip register.
 *
 * @param dev: pointer to device data.
 * @param reg: register address.
 * @return register value.
 */
uint16_t bk1080_readReg(const struct BK1080 *dev, enum bk1080Reg reg);

/**
 * Write a chip register.
 *
 * @param dev: pointer to device data.
 * @param reg: register address.
 * @param value: value to be written.
 */
void bk1080_writeReg(const struct BK1080 *dev, enum bk1080Reg reg,
                     uint16_t value);

/**
 * Get the lower frequency limit of a band.
 *
 * @param band: frequency band.
 * @return lower limit, in Hz.
 */
uint32_t bk1080_bandMinFreq(enum bk1080Band band);

/**
 * Get the upper frequency limit of a band.
 *
 * @param band: frequency band.
 * @return upper limit, in Hz.
 */
uint32_t bk1080_bandMaxFreq(enum bk1080Band band);

/**
 * Select the frequency band. Retune afterwards, the channel number is
 * relative to the start of the band.
 *
 * @param dev: pointer to device data.
 * @param band: frequency band.
 */
void bk1080_setBand(const struct BK1080 *dev, enum bk1080Band band);

/**
 * Get the selected frequency band.
 *
 * @param dev: pointer to device data.
 * @return frequency band.
 */
enum bk1080Band bk1080_getBand(const struct BK1080 *dev);

/**
 * Tune a frequency in the selected band. The frequency is rounded down to
 * the channel spacing.
 *
 * @param dev: pointer to device data.
 * @param freq: frequency, in Hz.
 * @return false if the frequency is outside the selected band.
 */
bool bk1080_setFrequency(const struct BK1080 *dev, uint32_t freq);

/**
 * Set the audio output volume.
 *
 * @param dev: pointer to device data.
 * @param volume: volume step, 0 to BK1080_VOLUME_MAX, higher is clamped.
 */
void bk1080_setVolume(const struct BK1080 *dev, uint8_t volume);

/**
 * Mute or unmute the audio output.
 *
 * @param dev: pointer to device data.
 * @param mute: true to mute.
 */
void bk1080_setMute(const struct BK1080 *dev, bool mute);

/**
 * Read the reception quality for the tuned frequency.
 *
 * @param dev: pointer to device data.
 * @param status: filled with the chip status.
 */
void bk1080_getStatus(const struct BK1080 *dev, struct bk1080Status *status);

#ifdef __cplusplus
}
#endif

#endif /* BK1080_H */
