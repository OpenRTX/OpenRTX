/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef BROADCAST_FM_H
#define BROADCAST_FM_H

#include <stdbool.h>
#include <stdint.h>
#include "core/datatypes.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Common interface for a broadcast FM receiver, available on platforms
 * defining CONFIG_BROADCAST_FM.
 *
 * The receiver is independent of the transceiver: while it is enabled the
 * platform routes its audio to the speaker. The caller must stop the
 * transceiver first (state.broadcastFm selects OPMODE_NONE) and enable the
 * receiver only once the transceiver reports being idle, so that the two
 * never drive the speaker at the same time.
 */

/**
 * Reception quality for the tuned frequency.
 */
typedef struct {
    uint8_t rssi; ///< Signal strength, in dBuV
    uint8_t snr;  ///< Signal to noise ratio, 0 (worst) to 15 (best)
    bool station; ///< A station is on the tuned frequency
    bool stereo;  ///< Stereo pilot detected, if the receiver reports it
} bcastfmStatus_t;

/**
 * Power on the receiver and route its audio to the speaker.
 *
 * @return true on success, false if the receiver did not answer.
 */
bool bcastfm_enable(void);

/**
 * Mute the receiver, power it off and release the speaker.
 */
void bcastfm_disable(void);

/**
 * Check whether the receiver is enabled.
 *
 * @return true if enabled.
 */
bool bcastfm_isEnabled(void);

/**
 * Get the frequency range of the receiver.
 *
 * @param min: filled with the lowest frequency, in Hz.
 * @param max: filled with the highest frequency, in Hz.
 */
void bcastfm_getBandLimits(freq_t *min, freq_t *max);

/**
 * Tune a frequency. Takes effect immediately; status readings settle after
 * about 100 ms.
 *
 * @param freq: frequency, in Hz.
 * @return false if the frequency is out of band or the receiver is off.
 */
bool bcastfm_setFrequency(freq_t freq);

/**
 * Read the reception quality for the tuned frequency.
 *
 * @param status: filled with the receiver status.
 */
void bcastfm_getStatus(bcastfmStatus_t *status);

#ifdef __cplusplus
}
#endif

#endif /* BROADCAST_FM_H */
