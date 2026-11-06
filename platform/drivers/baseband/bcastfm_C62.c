/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <stdlib.h>
#include <hwconfig.h>
#include <interfaces/delays.h>
#include <interfaces/broadcast_fm.h>
#include "drivers/audio/audio_c62.h"
#include "drivers/baseband/BK1080.h"
#include "drivers/I2C/i2c_bitbang.h"

/*
 * Broadcast FM receiver of the Retevis C62: a BK1080 whose audio output
 * reaches the speaker amplifier directly. Enabling it keeps the amplifier on.
 * The BK4819 audio output is muted by the radio driver when the transceiver
 * goes idle, see radio_setOpmode().
 */

#define FM_BAND BK1080_BAND_87_108

/* Station detection thresholds, from readings on a C62 */
#define STATION_MIN_RSSI 20U
#define STATION_MIN_SNR 4U
#define STATION_MAX_OFFSET 280

/* Delay between unmuting the receiver and enabling the amplifier */
#define AMP_DELAY_MS 100U

static bool busReady;
static bool enabled;

bool bcastfm_enable(void)
{
    if (enabled)
        return true;

    if (!busReady) {
        i2cBitbang_init(c62_bk1080.i2c);
        busReady = true;
    }

    if (!bk1080_init(&c62_bk1080)) {
        bk1080_terminate(&c62_bk1080);
        return false;
    }

    bk1080_setBand(&c62_bk1080, FM_BAND);
    bk1080_setVolume(&c62_bk1080, BK1080_VOLUME_MAX);

    bk1080_setMute(&c62_bk1080, false);
    delayMs(AMP_DELAY_MS);
    audio_c62_setBroadcastFm(true);

    enabled = true;
    return true;
}

void bcastfm_disable(void)
{
    if (!enabled)
        return;

    audio_c62_setBroadcastFm(false);
    bk1080_setMute(&c62_bk1080, true);
    bk1080_terminate(&c62_bk1080);
    enabled = false;
}

bool bcastfm_isEnabled(void)
{
    return enabled;
}

void bcastfm_getBandLimits(freq_t *min, freq_t *max)
{
    *min = bk1080_bandMinFreq(FM_BAND);
    *max = bk1080_bandMaxFreq(FM_BAND);
}

bool bcastfm_setFrequency(freq_t freq)
{
    if (!enabled)
        return false;

    return bk1080_setFrequency(&c62_bk1080, freq);
}

void bcastfm_getStatus(bcastfmStatus_t *status)
{
    struct bk1080Status st = { 0 };

    if (enabled)
        bk1080_getStatus(&c62_bk1080, &st);

    status->rssi = st.rssi;
    status->snr = st.snr;
    status->stereo = st.stereo;
    status->station = enabled && !st.afcRailed && st.rssi >= STATION_MIN_RSSI
                   && st.snr >= STATION_MIN_SNR
                   && abs(st.freqOffset) < STATION_MAX_OFFSET;
}
