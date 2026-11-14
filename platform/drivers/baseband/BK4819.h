/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * SPDX-FileCopyrightText: Copyright 2024 Jamiexu (Jamie793)
 * SPDX-FileCopyrightText: Copyright 2024 Andrej (Tunas1337)
 *
 * SPDX-License-Identifier: GPL-3.0-or-later AND MIT
 *
 * Based on Jamie793's MIT-licensed BK4819 driver for the Talkpod A36Plus,
 * https://github.com/Jamie793/A36Plus_Radio, as extended by Andrej in
 * https://github.com/OpenRTX/OpenRTX/pull/278.
 */

#ifndef BK4819_H
#define BK4819_H

#include <stdint.h>
#include <stdbool.h>
#include "peripherals/gpio.h"
#include "peripherals/spi.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BIT
#define BIT(n) (1U << (n))
#endif

/**
 * AF output modes, see bk4819_setAfOutput(). From the UV-K5 firmware:
 * https://github.com/DualTachyon/uv-k5-firmware/blob/6f8afac8864e0349ecf8b10f91ce8f69273013db/driver/bk4819.h#L24-L32
 * and, for bypass, the application note's RF_EnterBypass().
 */
enum bk4819_afOutput {
    BK4819_AF_MUTE = 0,   ///< Muted.
    BK4819_AF_FM = 1,     ///< FM demodulator.
    BK4819_AF_BEEP = 3,   ///< Tone generator.
    BK4819_AF_AM = 7,     ///< AM demodulator.
    BK4819_AF_BYPASS = 9, ///< Unfiltered discriminator, for digital modes.
};

/**
 * Audio processing modes, see bk4819_setAudioMode().
 */
enum bk4819_audioMode {
    BK4819_AUDIO_VOICE,  ///< Audio filters on, voice levels.
    BK4819_AUDIO_BYPASS, ///< Audio filters bypassed, for digital modes.
};

/**
 * GPIO pins, named after their use in the UV-K5:
 * https://github.com/DualTachyon/uv-k5-firmware/blob/6f8afac8864e0349ecf8b10f91ce8f69273013db/driver/bk4819-regs.h#L97-L104
 */
enum bk4819_gpio {
    BK4819_GPIO0_PIN28_RX_ENABLE = 0, ///< RX enable.
    BK4819_GPIO1_PIN29_PA_ENABLE = 1, ///< PA enable.
    BK4819_GPIO3_PIN31_UHF_LNA = 3,   ///< UHF LNA.
    BK4819_GPIO4_PIN32_VHF_LNA = 4,   ///< VHF LNA.
    BK4819_GPIO5_PIN1_RED = 5,        ///< Red LED.
    BK4819_GPIO6_PIN2_GREEN = 6,      ///< Green LED.
};

/**
 * BK4819 device data.
 *
 * Register access happens over the chip's 3-wire, SPI-like serial bus
 * (clock and bidirectional data), accessed through the generic struct
 * spiDevice interface of peripherals/spi.h. This makes the driver hardware
 * agnostic: any target can provide its own SPI implementation (e.g. a
 * bitbang driver) as long as it conforms to the spiDevice interface.
 *
 * Chip select is toggled directly by this driver, since the generic SPI
 * transfer function does not manage it.
 */
struct BK4819 {
    const struct spiDevice *spi; ///< SPI bus device
    struct gpioPin scn;          ///< Chip select, active low
};

/**
 * BK4819 GPIO device data.
 */
struct BK4819Priv {
    const struct BK4819 *bk4819; ///< Baseband device
    uint8_t activeLowMask;       ///< Per-pin active-low mask
};

/**
 * Instantiate a BK4819 GPIO device.
 *
 * Pins are numbered 0 through 6. Supported modes are INPUT, OUTPUT and
 * OUTPUT_ACTIVE_LOW. Both output modes start logically off: OUTPUT drives
 * low, OUTPUT_ACTIVE_LOW drives high. Active-low outputs invert set, clear
 * and read. Read samples the pin via REG0A. INPUT preserves the output latch.
 * Other modes return -ENOTSUP; invalid pins passed to
 * setMode return -EINVAL. Set, clear and read require a valid pin number.
 * Operations are not atomic.
 *
 * @param name: GPIO device name.
 * @param bk4819Dev: BK4819 device instance, not a pointer.
 */
#define GPIO_BK4819_DEVICE_DEFINE(name, bk4819Dev)                 \
    extern const struct gpioApi gpioBK4819_ops;                    \
    static struct BK4819Priv name##_priv = { .bk4819 = &bk4819Dev, \
                                             .activeLowMask = 0 }; \
    const struct gpioDev name = { .api = &gpioBK4819_ops,          \
                                  .priv = &name##_priv };

/**
 * Read a register.
 *
 * @param dev: pointer to device data.
 * @param reg: register address.
 * @return register value.
 */
uint16_t bk4819_readReg(const struct BK4819 *dev, uint8_t reg);

/**
 * Write a register.
 *
 * @param dev: pointer to device data.
 * @param reg: register address.
 * @param data: register value.
 */
void bk4819_writeReg(const struct BK4819 *dev, uint8_t reg, uint16_t data);

/**
 * Initialise the BK4819 chip.
 *
 * @param dev: pointer to device data.
 */
void bk4819_init(const struct BK4819 *dev);

/**
 * Set the frequency and turn on RX.
 *
 * @param dev: pointer to device data.
 * @param freq: frequency, in Hz.
 */
void bk4819_setFrequency(const struct BK4819 *dev, uint32_t freq);

/**
 * Turn on RX.
 *
 * @param dev: pointer to device data.
 */
void bk4819_enableRx(const struct BK4819 *dev);

/**
 * Turn on TX.
 *
 * @param dev: pointer to device data.
 */
void bk4819_enableTx(const struct BK4819 *dev);

/**
 * Turn off RX and TX.
 *
 * @param dev: pointer to device data.
 */
void bk4819_disableRtx(const struct BK4819 *dev);

/**
 * Select the AM or FM demodulator.
 *
 * @param dev: pointer to device data.
 * @param fm: true for FM, false for AM.
 */
void bk4819_setModulation(const struct BK4819 *dev, bool fm);

/**
 * Set the AF output mode.
 *
 * @param dev: pointer to device data.
 * @param af: AF output mode.
 */
void bk4819_setAfOutput(const struct BK4819 *dev, enum bk4819_afOutput af);

/**
 * Enable the TX CTCSS tone.
 *
 * @param dev: pointer to device data.
 * @param freq: tone frequency, in 0.1 Hz.
 */
void bk4819_enableTxCtcss(const struct BK4819 *dev, uint16_t freq);

/**
 * Enable the RX CTCSS detector.
 *
 * @param dev: pointer to device data.
 * @param freq: tone frequency, in 0.1 Hz.
 */
void bk4819_enableRxCtcss(const struct BK4819 *dev, uint16_t freq);

/**
 * Enable the TX CDCSS code.
 *
 * @param dev: pointer to device data.
 * @param negative: 0 for a positive code, 1 for a negative code.
 * @param bits24: 0 for a 23-bit code, 1 for a 24-bit code.
 * @param code: CDCSS code.
 */
void bk4819_enableTxCdcss(const struct BK4819 *dev, uint8_t negative,
                          uint8_t bits24, uint32_t code);

/**
 * Disable the TX CTCSS/CDCSS.
 *
 * @param dev: pointer to device data.
 */
void bk4819_disableCtdcss(const struct BK4819 *dev);

/**
 * Get the CTCSS detector flag.
 *
 * @param dev: pointer to device data.
 * @return non-zero when the tone is detected.
 */
uint16_t bk4819_getCtcss(const struct BK4819 *dev);

/**
 * Enable VOX.
 *
 * @param dev: pointer to device data.
 * @param delay: detection delay.
 * @param interval: detection interval.
 * @param thresholdOn: voice amplitude threshold for VOX on.
 * @param thresholdOff: voice amplitude threshold for VOX off.
 */
void bk4819_enableVox(const struct BK4819 *dev, uint8_t delay, uint8_t interval,
                      uint16_t thresholdOn, uint16_t thresholdOff);

/**
 * Get the VOX flag.
 *
 * @param dev: pointer to device data.
 * @return non-zero when voice is detected.
 */
uint8_t bk4819_getVox(const struct BK4819 *dev);

/**
 * Set the squelch thresholds.
 *
 * @param dev: pointer to device data.
 * @param rssiOpen: RSSI threshold to open, 0.5 dB/step.
 * @param rssiClose: RSSI threshold to close, 0.5 dB/step.
 * @param noiseOpen: ex-noise threshold to open.
 * @param noiseClose: ex-noise threshold to close.
 * @param glitchOpen: glitch threshold to open.
 * @param glitchClose: glitch threshold to close.
 */
void bk4819_setSquelch(const struct BK4819 *dev, uint8_t rssiOpen,
                       uint8_t rssiClose, uint8_t noiseOpen, uint8_t noiseClose,
                       uint8_t glitchOpen, uint8_t glitchClose);

/**
 * Set the audio processing mode. VOICE also sets the TX deviation, filters
 * and gains to the values bk4819_init() uses; BYPASS, as the application
 * note's RF_EnterBypass(), leaves them to the caller. The RX output is chosen
 * with bk4819_setAfOutput() (BK4819_AF_BYPASS in BYPASS), as it is also the
 * squelch mute.
 *
 * @param dev: pointer to device data.
 * @param mode: audio processing mode.
 */
void bk4819_setAudioMode(const struct BK4819 *dev, enum bk4819_audioMode mode);

/**
 * Set the RF TX deviation (REG_40).
 *
 * @param dev: pointer to device data.
 * @param enable: true to enable the deviation tuning.
 * @param tuning: raw tuning value, 0 to 4095 (not Hz).
 */
void bk4819_setTxDeviation(const struct BK4819 *dev, bool enable,
                           uint16_t tuning);

/**
 * Set the RX RF filter bandwidths (REG_43). Doubled in 25 kHz channel mode,
 * see bk4819_setFilterBandwidth().
 *
 * @param dev: pointer to device data.
 * @param bw: bandwidth, 0 to 7 (1.7 to 4.5 kHz).
 * @param weakBw: bandwidth when the signal is weak, 0 to 7.
 */
void bk4819_setRfFilter(const struct BK4819 *dev, uint8_t bw, uint8_t weakBw);

/**
 * Set the TX audio low-pass filter bandwidth (REG_43 AF TX LPF2).
 *
 * @param dev: pointer to device data.
 * @param bw: bandwidth, 0 to 7 (0 = 3 kHz, 1 = 2.5 kHz, see the register
 * list).
 */
void bk4819_setTxAudioFilter(const struct BK4819 *dev, uint8_t bw);

/**
 * Set the bandwidth mode (REG_43).
 *
 * @param dev: pointer to device data.
 * @param bandwidth: 0 for 12.5 kHz, any other value for 25 kHz.
 */
void bk4819_setFilterBandwidth(const struct BK4819 *dev, uint8_t bandwidth);

/**
 * Set the RX audio gains (REG_48).
 *
 * @param dev: pointer to device data.
 * @param gain1: gain 1, 0 to 3.
 * @param gain2: gain 2, 0 to 63.
 */
void bk4819_setRxAudioGain(const struct BK4819 *dev, uint8_t gain1,
                           uint8_t gain2);

/**
 * Set the microphone sensitivity (REG_7D).
 *
 * @param dev: pointer to device data.
 * @param gain: 0 to 31.
 */
void bk4819_setMicGain(const struct BK4819 *dev, uint8_t gain);

/**
 * Get the RSSI.
 *
 * @param dev: pointer to device data.
 * @return RSSI, in dBm.
 */
int16_t bk4819_getRssi(const struct BK4819 *dev);

/**
 * Get the microphone input level.
 *
 * @param dev: pointer to device data.
 * @return microphone level.
 */
uint16_t bk4819_getMicLevel(const struct BK4819 *dev);

/**
 * Start the beeper.
 *
 * @param dev: pointer to device data.
 * @param freq: beep frequency, in Hz.
 * @param tuningGain: tuning gain switch, unused.
 */
void bk4819_beepStart(const struct BK4819 *dev, uint16_t freq, bool tuningGain);

/**
 * Stop the beeper.
 *
 * @param dev: pointer to device data.
 */
void bk4819_beepStop(const struct BK4819 *dev);

#ifdef __cplusplus
}
#endif

#endif /* BK4819_H */
