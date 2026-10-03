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

#ifdef __cplusplus
extern "C" {
#endif

enum BK1080_Register_t {
    BK1080_REG_00 = 0x00U,
    BK1080_REG_02_POWER_CONFIGURATION = 0x02U,
    BK1080_REG_03_CHANNEL = 0x03U,
    BK1080_REG_05_SYSTEM_CONFIGURATION2 = 0x05U,
    BK1080_REG_07 = 0x07U,
    BK1080_REG_10 = 0x0AU,
    BK1080_REG_25_INTERNAL = 0x19U,
};

typedef enum BK1080_Register_t BK1080_Register_t;

// REG 07

#define BK1080_REG_07_SHIFT_FREQD 4
#define BK1080_REG_07_SHIFT_SNR 0

#define BK1080_REG_07_MASK_FREQD (0xFFFU << BK1080_REG_07_SHIFT_FREQD)
#define BK1080_REG_07_MASK_SNR (0x00FU << BK1080_REG_07_SHIFT_SNR)

#define BK1080_REG_07_GET_FREQD(x) \
    (((x) & BK1080_REG_07_MASK_FREQD) >> BK1080_REG_07_SHIFT_FREQD)
#define BK1080_REG_07_GET_SNR(x) \
    (((x) & BK1080_REG_07_MASK_SNR) >> BK1080_REG_07_SHIFT_SNR)

// REG 10

#define BK1080_REG_10_SHIFT_AFCRL 12
#define BK1080_REG_10_SHIFT_RSSI 0

#define BK1080_REG_10_MASK_AFCRL (0x01U << BK1080_REG_10_SHIFT_AFCRL)
#define BK1080_REG_10_MASK_RSSI (0xFFU << BK1080_REG_10_SHIFT_RSSI)

#define BK1080_REG_10_AFCRL_NOT_RAILED (0U << BK1080_REG_10_SHIFT_AFCRL)
#define BK1080_REG_10_AFCRL_RAILED (1U << BK1080_REG_10_SHIFT_AFCRL)

#define BK1080_REG_10_GET_RSSI(x) \
    (((x) & BK1080_REG_10_MASK_RSSI) >> BK1080_REG_10_SHIFT_RSSI)

/**
 * BK1080 device data.
 *
 * The serial interface towards the chip is bit-banged in software: clock
 * and bidirectional data implement the I2C-like protocol, while the power
 * pin switches the chip on and off.
 */
struct BK1080 {
    struct gpioPin sck; ///< Serial clock
    struct gpioPin sda; ///< Serial data, bidirectional
    struct gpioPin pwr; ///< Power switch, active low
};

/**
 * Base frequency and frequency deviation of the last measurement, in Hz.
 */
extern uint16_t BK1080_BaseFrequency;
extern uint16_t BK1080_FrequencyDeviation;

/**
 * Initialise the chip and tune a given frequency.
 *
 * @param dev: pointer to device data.
 * @param Frequency frequency to tune, in Hz, 0 to switch the chip off.
 * @param band frequency band selector.
 */
void BK1080_Init(const struct BK1080 *dev, uint32_t Frequency, uint8_t band);

/**
 * Read a chip register.
 *
 * @param dev: pointer to device data.
 * @param Register register address.
 * @return register value.
 */
uint16_t BK1080_ReadRegister(const struct BK1080 *dev,
                             BK1080_Register_t Register);

/**
 * Write a chip register.
 *
 * @param dev: pointer to device data.
 * @param Register register address.
 * @param Value value to be written.
 */
void BK1080_WriteRegister(const struct BK1080 *dev, BK1080_Register_t Register,
                          uint16_t Value);

/**
 * Mute or unmute the audio output.
 *
 * @param dev: pointer to device data.
 * @param Mute true to mute, false to unmute.
 */
void BK1080_Mute(const struct BK1080 *dev, bool Mute);

/**
 * Get the lower frequency limit of a given band.
 *
 * @param band band selector.
 * @return lower limit, in Hz.
 */
uint32_t BK1080_GetFreqLoLimit(uint8_t band);

/**
 * Get the upper frequency limit of a given band.
 *
 * @param band band selector.
 * @return upper limit, in Hz.
 */
uint32_t BK1080_GetFreqHiLimit(uint8_t band);

/**
 * Tune a given frequency.
 *
 * @param dev: pointer to device data.
 * @param frequency frequency to tune, in Hz.
 * @param band frequency band selector.
 */
void BK1080_SetFrequency(const struct BK1080 *dev, uint32_t frequency,
                         uint8_t band);

/**
 * Measure the frequency deviation around a given frequency.
 *
 * @param dev: pointer to device data.
 * @param Frequency center frequency, in Hz.
 */
void BK1080_GetFrequencyDeviation(const struct BK1080 *dev, uint32_t Frequency);

#ifdef __cplusplus
}
#endif

#endif /* BK1080_H */
