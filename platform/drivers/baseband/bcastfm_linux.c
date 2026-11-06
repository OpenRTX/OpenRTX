/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <stdio.h>
#include <interfaces/broadcast_fm.h>

/*
 * Simulated broadcast FM receiver for the Linux emulator: a few stations
 * with fixed signal levels, so the FM radio screen can be used without
 * hardware. No audio is produced.
 */

#define BAND_MIN 87500000U
#define BAND_MAX 108000000U

struct station {
    freq_t freq;
    uint8_t rssi;
    uint8_t snr;
};

static const struct station stations[] = {
    { 88300000, 30, 7 },
    { 94500000, 45, 12 },
    { 101100000, 38, 10 },
    { 104100000, 25, 5 },
};

static bool enabled;
static freq_t tuned;

bool bcastfm_enable(void)
{
    enabled = true;
    printf("Broadcast FM: on\n");
    return true;
}

void bcastfm_disable(void)
{
    enabled = false;
    printf("Broadcast FM: off\n");
}

bool bcastfm_isEnabled(void)
{
    return enabled;
}

void bcastfm_getBandLimits(freq_t *min, freq_t *max)
{
    *min = BAND_MIN;
    *max = BAND_MAX;
}

bool bcastfm_setFrequency(freq_t freq)
{
    if (!enabled || freq < BAND_MIN || freq > BAND_MAX)
        return false;

    tuned = freq;
    printf("Broadcast FM: tuned %lu Hz\n", (unsigned long)freq);
    return true;
}

void bcastfm_getStatus(bcastfmStatus_t *status)
{
    status->rssi = 8;
    status->snr = 1;
    status->station = false;
    status->stereo = false;

    if (!enabled)
        return;

    for (unsigned int i = 0; i < sizeof(stations) / sizeof(stations[0]); i++) {
        if (stations[i].freq == tuned) {
            status->rssi = stations[i].rssi;
            status->snr = stations[i].snr;
            status->station = true;
            status->stereo = true;
            return;
        }
    }
}
