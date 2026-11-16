/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef DSP_H
#define DSP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * This header contains various DSP utilities which can be used to condition
 * input or output signals when implementing digital modes on OpenRTX.
 */

/**
 * Reset the state variables of a DSP object.
 *
 * @param state: state variable.
 */
#define dsp_resetState(state) memset(&state, 0x00, sizeof(state))

/**
 * Data structure holding the internal state of a DC blocking filter.
 */
struct dcBlock {
    int32_t accum;
    int32_t prevIn;
    int32_t prevOut;
    bool initialized; ///< Primed on the first sample after a reset
};

/**
 * Run a single step of the DC blocking filter. The first sample after a reset
 * primes the filter, so a signal starting on a DC level passes without a step.
 *
 * @param dcb: pointer to DC filter state.
 * @param sample: input sample.
 * @return filtered sample
 */
int16_t dsp_dcBlockFilter(struct dcBlock *dcb, int16_t sample);

/**
 * Remove the DC offset from a collection of samples, processing data in-place.
 *
 * @param state: pointer to the data structure containing the filter state.
 * @param buffer: buffer containing the audio samples.
 * @param length: number of samples contained in the buffer.
 */
static inline void dsp_removeDcOffset(struct dcBlock *dcb, int16_t *buffer,
                                      const size_t length)
{
    for (size_t i = 0; i < length; i++)
        buffer[i] = dsp_dcBlockFilter(dcb, buffer[i]);
}

/**
 * Data structure holding the internal state of a decimation block.
 */
struct decimatorState {
    int32_t accumulator;
    uint32_t count;
};

/**
 * @brief Run a single step of the oversampling decimation filter.
 *
 * Accumulates input samples and returns their truncated integer average
 * once the oversampling factor number of samples have been collected.
 *
 * @param state: pointer to the decimator state.
 * @param sample: pointer to the audio sample. Contains the value of the
 * decimated output when the function returns true.
 * @param ratio: sample decimation ratio.
 * @return true when a decimated sample is ready, false otherwise.
 */
bool dsp_decimator(struct decimatorState *state, int16_t *sample,
                   uint16_t ratio);

/**
 * Data structure holding the internal state of a low pass decimator. Set it
 * up with dsp_lowPassInit() and release it with dsp_lowPassTerminate(), not
 * with dsp_resetState().
 */
struct lowPassDecimator {
    void *fir;        ///< Filter for the ratio, owned by the decimator.
    uint8_t ratio;    ///< Decimation ratio, 0 when not set up.
    uint8_t count;    ///< Inputs since the last output.
    bool initialized; ///< History primed on the first sample.
};

/**
 * Set up a low pass decimator for a decimation ratio, allocating its filter.
 * The first sample after this fills the filter's history, so a signal
 * starting on a DC level passes without a step. Release the decimator with
 * dsp_lowPassTerminate() before setting it up again.
 *
 * The filter has unity gain at DC, -2 dB at 0.375 and -6 dB at 0.4625 of the
 * output rate, and at least 70 dB of rejection from 0.75 of the output rate
 * up, so content above half the output rate does not fold into the band.
 * Ratio 6 takes 48 kHz to 8 kHz with 48 taps and ratio 8 64 kHz to 8 kHz
 * with 64 taps.
 *
 * @param state: pointer to the decimator state.
 * @param ratio: decimation ratio.
 * @return 0 on success, -ENOTSUP if there is no filter for the ratio or
 * -ENOMEM if the filter could not be allocated.
 */
int dsp_lowPassInit(struct lowPassDecimator *state, uint8_t ratio);

/**
 * Run a single step of the low pass decimator.
 *
 * @param state: pointer to the decimator state.
 * @param sample: pointer to the input sample. Contains the decimated output
 * when the function returns true.
 * @return true when a decimated sample is ready, false otherwise or if the
 * decimator is not set up.
 */
bool dsp_lowPassDecimate(struct lowPassDecimator *state, int16_t *sample);

/**
 * Release the filter of a low pass decimator. Safe to call after a failed
 * dsp_lowPassInit() or more than once.
 *
 * @param state: pointer to the decimator state.
 */
void dsp_lowPassTerminate(struct lowPassDecimator *state);

/**
 * Number of windows over which the AGC tracks the noise floor.
 */
#define DSP_AGC_WINDOWS 4

/**
 * Data structure holding the state of an automatic gain control, which
 * levels speech in blocks of 20 ms at 8 kHz. Levels are in dBFS.
 */
struct agc {
    float targetDb;                ///< Speech level aimed at
    float baseGain;                ///< Gain from samples to full scale (1.0)
    float gainDb;                  ///< Gain at the end of the last block
    float levelDb;                 ///< Speech level before the AGC, smoothed
    float floorDb;                 ///< Noise floor before the AGC
    float winMin[DSP_AGC_WINDOWS]; ///< Quietest block of each recent window
    uint16_t winCount;             ///< Blocks into the current window
    uint16_t run;                  ///< Consecutive blocks above the gate
    uint16_t acquire;              ///< Blocks left of faster rise
};

/**
 * Initialise an automatic gain control at a starting gain, as if the last
 * speech had needed it. During its first second of speech the gain may rise
 * faster than afterwards, so that the first transmission settles quickly; a
 * start near a radio's usual talker saves even that rise.
 *
 * @param agc: pointer to the AGC state.
 * @param targetDb: speech level to aim at, in dBFS. The level estimate rises
 * fast and falls slowly, so it follows the louder syllables: the speech's
 * RMS settles about 3 dB under the target.
 * @param baseGain: fixed gain applied to the samples before the AGC's own,
 * in units of full scale per sample (for example, mic gain / 32768).
 * @param startGainDb: the AGC's own gain to start at, in dB.
 */
void dsp_agcInit(struct agc *agc, float targetDb, float baseGain,
                 float startGainDb);

/**
 * Start a new transmission: forget the noise floor, which may have changed
 * since the last one, and keep the gain and the speech level.
 *
 * @param agc: pointer to the AGC state.
 */
void dsp_agcRestart(struct agc *agc);

/**
 * Level a block of 20 ms of 8 kHz samples in place, then limit its peaks
 * below -1 dBFS.
 *
 * The noise floor is the quietest block of the last 1.6 s, and a block more
 * than 5 dB above it counts as speech. The gain may fall on any speech block,
 * and on any block more than 6 dB over the target, but rises only after
 * 120 ms of speech in a row, so that bursts of noise and pauses do not pump
 * the background up. The gain is ramped across the block.
 *
 * @param agc: pointer to the AGC state.
 * @param block: samples, processed in place.
 * @param len: number of samples in the block, 160: the time constants are
 * set for blocks of 20 ms.
 */
void dsp_agcProcess(struct agc *agc, int16_t *block, size_t len);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* DSP_H */
