/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef BCASTFM_TUNER_H
#define BCASTFM_TUNER_H

#include <stdbool.h>
#include <stdint.h>
#include "core/datatypes.h"
#include "interfaces/broadcast_fm.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Broadcast FM tuning logic: channel stepping, seek and direct frequency
 * entry, on top of the broadcast FM interface. Hardware independent; the UI
 * drives it with key presses and a periodic tick.
 */

/** Channel step, in Hz. */
#define BCASTFM_STEP 100000U

/** Maximum number of digits of a typed frequency (e.g. 1 0 1 1). */
#define BCASTFM_MAX_DIGITS 4

typedef struct {
    freq_t freq;         ///< Tuned frequency, in Hz
    freq_t min;          ///< Lowest frequency of the band, in Hz
    freq_t max;          ///< Highest frequency of the band, in Hz
    int8_t seekDir;      ///< Seek direction: +1 up, -1 down, 0 idle
    freq_t seekStart;    ///< Frequency the seek started from
    uint8_t inputDigits; ///< Number of typed digits, 0 when not typing
    uint8_t input[BCASTFM_MAX_DIGITS]; ///< Typed digits
    bcastfmStatus_t status;            ///< Last receiver status
} bcastfmTuner_t;

/**
 * Initialise the tuner state and tune a frequency, clamped to the band.
 * The receiver must already be enabled.
 *
 * @param tuner: tuner state.
 * @param freq: frequency to tune, in Hz.
 */
void bcastfmTuner_init(bcastfmTuner_t *tuner, freq_t freq);

/**
 * Tune a frequency, rounded to the channel step and clamped to the band.
 * Stops a running seek and discards typed digits.
 *
 * @param tuner: tuner state.
 * @param freq: frequency, in Hz.
 */
void bcastfmTuner_tune(bcastfmTuner_t *tuner, freq_t freq);

/**
 * Move one channel up or down, wrapping around at the band edges.
 *
 * @param tuner: tuner state.
 * @param dir: +1 for up, -1 for down.
 */
void bcastfmTuner_step(bcastfmTuner_t *tuner, int dir);

/**
 * Start seeking the next station in a direction.
 *
 * @param tuner: tuner state.
 * @param dir: +1 for up, -1 for down.
 */
void bcastfmTuner_startSeek(bcastfmTuner_t *tuner, int dir);

/**
 * Stop a running seek, staying on the current frequency.
 *
 * @param tuner: tuner state.
 */
void bcastfmTuner_stopSeek(bcastfmTuner_t *tuner);

/**
 * Periodic update, to be called about every 100 ms: reads the receiver
 * status and advances a running seek by one channel. A seek stops on a
 * station or after a full turn around the band.
 *
 * @param tuner: tuner state.
 */
void bcastfmTuner_tick(bcastfmTuner_t *tuner);

/**
 * Add a digit to the typed frequency. Frequencies are typed in MHz with an
 * implied decimal digit: 1 0 1 1 is 101.1 MHz, 8 8 5 is 88.5 MHz. The
 * frequency is tuned as soon as it is complete.
 *
 * @param tuner: tuner state.
 * @param digit: digit, 0 to 9.
 */
void bcastfmTuner_inputDigit(bcastfmTuner_t *tuner, uint8_t digit);

/**
 * Tune the typed frequency now, if it is in the band.
 *
 * @param tuner: tuner state.
 * @return true if a frequency was tuned.
 */
bool bcastfmTuner_inputConfirm(bcastfmTuner_t *tuner);

/**
 * Discard the typed digits.
 *
 * @param tuner: tuner state.
 */
void bcastfmTuner_inputCancel(bcastfmTuner_t *tuner);

/**
 * Get the typed frequency so far, in Hz, with missing digits as zero.
 *
 * @param tuner: tuner state.
 * @return frequency, in Hz.
 */
freq_t bcastfmTuner_inputFrequency(const bcastfmTuner_t *tuner);

/**
 * Get the number of digits a typed frequency needs, from its first digit:
 * four for 100 MHz and up, three otherwise.
 *
 * @param tuner: tuner state.
 * @return number of digits, 0 when not typing.
 */
uint8_t bcastfmTuner_inputLength(const bcastfmTuner_t *tuner);

#ifdef __cplusplus
}
#endif

#endif /* BCASTFM_TUNER_H */
