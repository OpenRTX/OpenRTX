/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <hwconfig.h>

#ifdef CONFIG_BROADCAST_FM

#include <string.h>
#include "core/bcastfm_tuner.h"

/**
 * Round a frequency to the channel grid and clamp it to the band.
 */
static freq_t snapToBand(const bcastfmTuner_t *tuner, freq_t freq)
{
    if (freq <= tuner->min)
        return tuner->min;
    if (freq >= tuner->max)
        return tuner->max;

    freq_t channel = (freq - tuner->min + BCASTFM_STEP / 2) / BCASTFM_STEP;
    freq_t snapped = tuner->min + channel * BCASTFM_STEP;

    return (snapped > tuner->max) ? tuner->max : snapped;
}

/**
 * Get the next channel in a direction, wrapping around at the band edges.
 */
static freq_t nextChannel(const bcastfmTuner_t *tuner, int dir)
{
    if (dir > 0)
        return (tuner->freq >= tuner->max) ? tuner->min :
                                             tuner->freq + BCASTFM_STEP;

    return (tuner->freq <= tuner->min) ? tuner->max :
                                         tuner->freq - BCASTFM_STEP;
}

/**
 * Tune a frequency already on the channel grid, keeping seek and input state.
 */
static void setFrequency(bcastfmTuner_t *tuner, freq_t freq)
{
    tuner->freq = freq;
    bcastfm_setFrequency(freq);
}

void bcastfmTuner_init(bcastfmTuner_t *tuner, freq_t freq)
{
    memset(tuner, 0, sizeof(*tuner));
    bcastfm_getBandLimits(&tuner->min, &tuner->max);
    bcastfmTuner_tune(tuner, freq);
}

void bcastfmTuner_tune(bcastfmTuner_t *tuner, freq_t freq)
{
    tuner->seekDir = 0;
    tuner->inputDigits = 0;
    setFrequency(tuner, snapToBand(tuner, freq));
}

void bcastfmTuner_step(bcastfmTuner_t *tuner, int dir)
{
    bcastfmTuner_tune(tuner, nextChannel(tuner, dir));
}

void bcastfmTuner_startSeek(bcastfmTuner_t *tuner, int dir)
{
    tuner->inputDigits = 0;
    tuner->seekDir = (dir > 0) ? 1 : -1;
    tuner->seekStart = tuner->freq;
    setFrequency(tuner, nextChannel(tuner, tuner->seekDir));
}

void bcastfmTuner_stopSeek(bcastfmTuner_t *tuner)
{
    tuner->seekDir = 0;
}

void bcastfmTuner_tick(bcastfmTuner_t *tuner)
{
    bcastfm_getStatus(&tuner->status);

    if (tuner->seekDir == 0)
        return;

    if (tuner->status.station) {
        tuner->seekDir = 0;
        return;
    }

    freq_t next = nextChannel(tuner, tuner->seekDir);
    if (next == tuner->seekStart) {
        /* Full turn without a station, go back to where we started */
        bcastfmTuner_tune(tuner, tuner->seekStart);
        return;
    }

    setFrequency(tuner, next);
}

uint8_t bcastfmTuner_inputLength(const bcastfmTuner_t *tuner)
{
    if (tuner->inputDigits == 0)
        return 0;

    return (tuner->input[0] == 1) ? 4 : 3;
}

freq_t bcastfmTuner_inputFrequency(const bcastfmTuner_t *tuner)
{
    uint8_t len = bcastfmTuner_inputLength(tuner);
    freq_t value = 0;

    /* Value in units of 100 kHz, missing digits as zero */
    for (uint8_t i = 0; i < len; i++) {
        value *= 10;
        if (i < tuner->inputDigits)
            value += tuner->input[i];
    }

    return value * 100000U;
}

void bcastfmTuner_inputDigit(bcastfmTuner_t *tuner, uint8_t digit)
{
    if (digit > 9)
        return;

    tuner->seekDir = 0;
    tuner->input[tuner->inputDigits++] = digit;

    if (tuner->inputDigits >= bcastfmTuner_inputLength(tuner))
        bcastfmTuner_inputConfirm(tuner);
}

bool bcastfmTuner_inputConfirm(bcastfmTuner_t *tuner)
{
    if (tuner->inputDigits == 0)
        return false;

    freq_t freq = bcastfmTuner_inputFrequency(tuner);
    tuner->inputDigits = 0;

    if (freq < tuner->min || freq > tuner->max)
        return false;

    bcastfmTuner_tune(tuner, freq);
    return true;
}

void bcastfmTuner_inputCancel(bcastfmTuner_t *tuner)
{
    tuner->inputDigits = 0;
}

#endif /* CONFIG_BROADCAST_FM */
