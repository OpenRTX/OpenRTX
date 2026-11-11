/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <cstdint>
#include <limits>

#include "core/dsp.h"

/*
 * Two's-complement equivalent of the old `sample << 15` without
 * invoking signed left-shift UB.
 */
static int32_t prev_in_old(int16_t sample)
{
    return static_cast<int32_t>(
        static_cast<uint32_t>(static_cast<int32_t>(sample)) << 15);
}

/*
 * Reference DC block filter using the old left-shift scaling form,
 * expressed without signed left-shift UB.
 */
static int16_t dc_block_old(struct dcBlock *dcb, int16_t sample)
{
    dcb->accum -= dcb->prevIn;
    dcb->prevIn = prev_in_old(sample);
    dcb->accum += dcb->prevIn;
    dcb->accum -= 164 * dcb->prevOut;
    dcb->prevOut = dcb->accum >> 15;

    return static_cast<int16_t>(dcb->prevOut);
}

TEST_CASE("DC block prevIn matches two's-complement shift for all int16_t",
          "[dsp][dcblock]")
{
    struct dcBlock dcb;

    for (int v = std::numeric_limits<int16_t>::min();
         v <= std::numeric_limits<int16_t>::max(); v++) {
        const int16_t sample = static_cast<int16_t>(v);

        dsp_resetState(dcb);
        (void)dsp_dcBlockFilter(&dcb, sample);
        REQUIRE(dcb.prevIn == prev_in_old(sample));
    }
}

TEST_CASE("DC block filter output matches old unsigned-shift reference",
          "[dsp][dcblock]")
{
    static const int16_t sequence[] = {
        -1,
        -60,
        -1000,
        std::numeric_limits<int16_t>::min(),
        std::numeric_limits<int16_t>::max(),
        0,
        1000,
        5000, /* DC offset held briefly */
        5000,
        5000,
        5000,
        /* low-frequency sine samples around a DC offset */
        5000,
        6535,
        7694,
        8264,
        8129,
        7300,
        5885,
        4114,
        2221,
        444,
        -1106,
        -2221,
        -2792,
        -2657,
        -1828,
        -413,
        1300,
        3071,
        4685,
        5800,
        6364,
        6364,
    };

    struct dcBlock live;
    struct dcBlock ref;

    dsp_resetState(live);
    dsp_resetState(ref);

    for (size_t i = 0; i < sizeof(sequence) / sizeof(sequence[0]); i++) {
        const int16_t out = dsp_dcBlockFilter(&live, sequence[i]);
        const int16_t expected = dc_block_old(&ref, sequence[i]);

        REQUIRE(out == expected);
        REQUIRE(live.accum == ref.accum);
        REQUIRE(live.prevIn == ref.prevIn);
        REQUIRE(live.prevOut == ref.prevOut);
    }
}

TEST_CASE("DC block filter removes a constant negative offset",
          "[dsp][dcblock]")
{
    struct dcBlock dcb;
    dsp_resetState(dcb);

    const int16_t dc = -5000;
    int16_t last = dc;

    for (int i = 0; i < 20000; i++)
        last = dsp_dcBlockFilter(&dcb, dc);

    REQUIRE(std::abs(static_cast<int>(last)) < 50);
}
