/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef C6000_MODEM_H
#define C6000_MODEM_H

#include "interfaces/audio.h"

/**
 * Baseband stream drivers for radios that send M17 either through an analog
 * path or through the HR_C6000's own modem:
 * - output: a stream at the symbol rate goes to the HR_C6000 symbol output
 *   driver, any other to the analog path;
 * - input: the HR_C6000 FM receive copy when the radio has picked the chip
 *   (C6000modem_useChip()), the analog path otherwise;
 * - microphone, for radios whose microphone reaches the MCU without a mod
 *   only through the HR_C6000's codec: the codec when the radio has picked
 *   the chip, the analog path otherwise. The stream has the rate of the
 *   codec's samples: the analog path is oversampled and low pass filtered
 *   down to it. Each path has its gain.
 *
 * The configuration of the output and input devices names their analog path,
 * a struct C6000modemPath; the microphone's names both paths, a struct
 * C6000micPaths. Each stream keeps the driver it started with.
 */

/**
 * Analog baseband path of a device.
 */
struct C6000modemPath {
    const struct audioDriver *driver; ///< Driver of the analog path
    const void *config;               ///< Its configuration
};

/**
 * Microphone paths of a device.
 */
struct C6000micPaths {
    struct C6000modemPath chip;   ///< Through the HR_C6000's codec
    uint8_t chipGain;             ///< Gain of the codec's samples
    struct C6000modemPath analog; ///< The analog path
    uint8_t analogRatio;          ///< Oversampling of the analog path
    uint8_t analogGain;           ///< Gain of the filtered samples
};

/**
 * Pick the receive baseband and microphone sources for the next input
 * streams.
 *
 * @param chip: true for the HR_C6000 FM receive copy and codec, false for the
 * analog paths.
 */
void C6000modem_useChip(const bool chip);

extern "C" const struct audioDriver C6000_modem_out_audio_driver;
extern "C" const struct audioDriver C6000_modem_in_audio_driver;
extern "C" const struct audioDriver C6000_modem_mic_audio_driver;

#endif /* C6000_MODEM_H */
