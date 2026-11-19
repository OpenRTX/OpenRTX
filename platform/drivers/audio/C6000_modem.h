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
 *   (C6000modem_useChip()), the analog path otherwise.
 *
 * The configuration of each device names its analog path, a struct
 * C6000modemPath. Each stream keeps the driver it started with.
 */

/**
 * Analog baseband path of a device.
 */
struct C6000modemPath {
    const struct audioDriver *driver; ///< Driver of the analog path
    const void *config;               ///< Its configuration
};

/**
 * Pick the receive baseband source for the next input streams.
 *
 * @param chip: true for the HR_C6000 FM receive copy, false for the analog
 * path.
 */
void C6000modem_useChip(const bool chip);

extern "C" const struct audioDriver C6000_modem_out_audio_driver;
extern "C" const struct audioDriver C6000_modem_in_audio_driver;

#endif /* C6000_MODEM_H */
