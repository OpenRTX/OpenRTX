/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "protocols/APRS/Slicer.hpp"

using namespace APRS;

Slicer::Slicer()
{
}

Slicer::~Slicer()
{
}

bool Slicer::getDCD()
{
    if (dcdBitTimer > 0)
        return true;
    return false;
}

int8_t Slicer::slice(const int16_t input)
{
    int8_t result = -1;

    clock++;
    // check for symbol center
    if (clock >= ROLLOVER_THRESH) {
        // at or past symbol center, reset clock
        clock -= SAMPLES_PER_SYMBOL;
        if (dcdBitTimer > 0) {
            dcdBitTimer--;
        }
        result = input >= 0 ? 1 : 0;
    }
    // check for zero-crossing in sample stream
    if ((lastSample ^ input) < 0) {
        // the clock should be zero at the crossing otherwise nudge it
        if (abs(clock) >= NUDGE_THRESH) {
            if (clock > 0)
                clock -= NUDGE;
            else
                clock += NUDGE;
            dcdCrossings = 0;
        } else {
            dcdCrossings++;
            if (dcdCrossings >= DCD_THRESH) {
                dcdBitTimer = DCD_BITS;
                dcdCrossings = 0;
            }
        }
    }
    lastSample = input;
    return result;
}
