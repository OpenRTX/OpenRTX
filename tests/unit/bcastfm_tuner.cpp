/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <catch2/catch_test_macros.hpp>
#include <set>
#include <vector>
#include "core/bcastfm_tuner.h"

namespace
{

/**
 * Fake receiver: 87.5 - 108 MHz, with stations at the frequencies in
 * `stations`. Records every tuned frequency.
 */
struct FakeReceiver {
    std::set<freq_t> stations;
    std::vector<freq_t> tuned;
};

FakeReceiver rx;

void resetReceiver(std::set<freq_t> stations = {})
{
    rx = FakeReceiver();
    rx.stations = stations;
}

} // namespace

extern "C" {

bool bcastfm_enable(void)
{
    return true;
}

void bcastfm_disable(void)
{
}

bool bcastfm_isEnabled(void)
{
    return true;
}

void bcastfm_getBandLimits(freq_t *min, freq_t *max)
{
    *min = 87500000;
    *max = 108000000;
}

bool bcastfm_setFrequency(freq_t freq)
{
    rx.tuned.push_back(freq);
    return true;
}

void bcastfm_getStatus(bcastfmStatus_t *status)
{
    bool station = !rx.tuned.empty() && rx.stations.count(rx.tuned.back());
    status->rssi = station ? 40 : 5;
    status->snr = station ? 10 : 0;
    status->station = station;
    status->stereo = false;
}

} // extern "C"

TEST_CASE("init clamps and snaps the frequency", "[fm][tuner]")
{
    bcastfmTuner_t t;

    resetReceiver();
    bcastfmTuner_init(&t, 101100000);
    REQUIRE(t.freq == 101100000);
    REQUIRE(rx.tuned.back() == 101100000);

    bcastfmTuner_init(&t, 101140000);
    REQUIRE(t.freq == 101100000);
    bcastfmTuner_init(&t, 101160000);
    REQUIRE(t.freq == 101200000);

    bcastfmTuner_init(&t, 50000000);
    REQUIRE(t.freq == 87500000);
    bcastfmTuner_init(&t, 0);
    REQUIRE(t.freq == 87500000);
    bcastfmTuner_init(&t, 150000000);
    REQUIRE(t.freq == 108000000);
}

TEST_CASE("step moves one channel and wraps at the band edges", "[fm][tuner]")
{
    bcastfmTuner_t t;
    resetReceiver();
    bcastfmTuner_init(&t, 101100000);

    bcastfmTuner_step(&t, +1);
    REQUIRE(t.freq == 101200000);
    bcastfmTuner_step(&t, -1);
    bcastfmTuner_step(&t, -1);
    REQUIRE(t.freq == 101000000);

    bcastfmTuner_tune(&t, 108000000);
    bcastfmTuner_step(&t, +1);
    REQUIRE(t.freq == 87500000);
    bcastfmTuner_step(&t, -1);
    REQUIRE(t.freq == 108000000);
}

TEST_CASE("seek stops on the next station", "[fm][tuner]")
{
    bcastfmTuner_t t;
    resetReceiver({ 94500000, 101100000 });
    bcastfmTuner_init(&t, 95000000);

    bcastfmTuner_startSeek(&t, +1);
    int ticks = 0;
    while (t.seekDir != 0 && ticks < 1000) {
        bcastfmTuner_tick(&t);
        ticks++;
    }
    REQUIRE(t.freq == 101100000);
    REQUIRE(t.status.station);

    /* Seeking again skips the station we are on */
    bcastfmTuner_startSeek(&t, -1);
    while (t.seekDir != 0 && ticks < 2000) {
        bcastfmTuner_tick(&t);
        ticks++;
    }
    REQUIRE(t.freq == 94500000);
}

TEST_CASE("seek wraps around the band", "[fm][tuner]")
{
    bcastfmTuner_t t;
    resetReceiver({ 88300000 });
    bcastfmTuner_init(&t, 107000000);

    bcastfmTuner_startSeek(&t, +1);
    for (int i = 0; i < 1000 && t.seekDir != 0; i++)
        bcastfmTuner_tick(&t);

    REQUIRE(t.freq == 88300000);
}

TEST_CASE("seek with no station returns to the start", "[fm][tuner]")
{
    bcastfmTuner_t t;
    resetReceiver();
    bcastfmTuner_init(&t, 99900000);

    bcastfmTuner_startSeek(&t, -1);
    int ticks = 0;
    while (t.seekDir != 0 && ticks < 1000) {
        bcastfmTuner_tick(&t);
        ticks++;
    }

    REQUIRE(t.seekDir == 0);
    REQUIRE(t.freq == 99900000);
    /* One full turn: every other channel visited once */
    REQUIRE(ticks == 205);
}

TEST_CASE("stepping or typing stops a seek", "[fm][tuner]")
{
    bcastfmTuner_t t;
    resetReceiver();
    bcastfmTuner_init(&t, 99900000);

    bcastfmTuner_startSeek(&t, +1);
    bcastfmTuner_step(&t, +1);
    REQUIRE(t.seekDir == 0);

    bcastfmTuner_startSeek(&t, +1);
    bcastfmTuner_inputDigit(&t, 9);
    REQUIRE(t.seekDir == 0);
}

TEST_CASE("typed frequencies tune when complete", "[fm][tuner]")
{
    bcastfmTuner_t t;
    resetReceiver();
    bcastfmTuner_init(&t, 99900000);

    /* Four digits from 100 MHz up */
    bcastfmTuner_inputDigit(&t, 1);
    bcastfmTuner_inputDigit(&t, 0);
    bcastfmTuner_inputDigit(&t, 1);
    REQUIRE(t.inputDigits == 3);
    REQUIRE(t.freq == 99900000);
    REQUIRE(bcastfmTuner_inputFrequency(&t) == 101000000);
    bcastfmTuner_inputDigit(&t, 1);
    REQUIRE(t.inputDigits == 0);
    REQUIRE(t.freq == 101100000);

    /* Three digits below 100 MHz */
    bcastfmTuner_inputDigit(&t, 8);
    REQUIRE(bcastfmTuner_inputLength(&t) == 3);
    bcastfmTuner_inputDigit(&t, 8);
    bcastfmTuner_inputDigit(&t, 5);
    REQUIRE(t.freq == 88500000);
}

TEST_CASE("typed frequencies can be confirmed early or cancelled",
          "[fm][tuner]")
{
    bcastfmTuner_t t;
    resetReceiver();
    bcastfmTuner_init(&t, 99900000);

    bcastfmTuner_inputDigit(&t, 1);
    bcastfmTuner_inputDigit(&t, 0);
    REQUIRE(bcastfmTuner_inputConfirm(&t));
    REQUIRE(t.freq == 100000000);

    bcastfmTuner_inputDigit(&t, 9);
    bcastfmTuner_inputCancel(&t);
    REQUIRE(t.inputDigits == 0);
    REQUIRE(t.freq == 100000000);
    REQUIRE_FALSE(bcastfmTuner_inputConfirm(&t));
}

TEST_CASE("typed frequencies outside the band are ignored", "[fm][tuner]")
{
    bcastfmTuner_t t;
    resetReceiver();
    bcastfmTuner_init(&t, 99900000);

    /* 80.0 and 109.9 MHz */
    bcastfmTuner_inputDigit(&t, 8);
    bcastfmTuner_inputDigit(&t, 0);
    bcastfmTuner_inputDigit(&t, 0);
    REQUIRE(t.freq == 99900000);
    REQUIRE(t.inputDigits == 0);

    bcastfmTuner_inputDigit(&t, 1);
    bcastfmTuner_inputDigit(&t, 0);
    bcastfmTuner_inputDigit(&t, 9);
    bcastfmTuner_inputDigit(&t, 9);
    REQUIRE(t.freq == 99900000);
}
