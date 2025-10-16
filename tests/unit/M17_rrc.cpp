/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <limits.h>
#include <inttypes.h>
#include <stdio.h>
#include "protocols/M17/M17DSP.hpp"

#define IMPULSE_SIZE 4096

using namespace std;

/**
 * Test the different demodulation steps
 */

int main()
{
    // Open file
    FILE *baseband_out = fopen("M17_rrc_impulse_response.raw", "wb");
    if (!baseband_out)
    {
        perror("Error in opening output file");
        return -1;
    }

    // Allocate impulse signal
    int16_t impulse[IMPULSE_SIZE] = { 0 };
    impulse[0] = SHRT_MAX;

    // Apply RRC on impulse signal
    int16_t filtered_impulse[IMPULSE_SIZE] = { 0 };
    for(size_t i = 0; i < IMPULSE_SIZE; i++)
    {
        float elem = static_cast< float >(impulse[i]);
        filtered_impulse[i] = static_cast< int16_t >(M17::rrc_48k(0.10 * elem));
    }
    fwrite(filtered_impulse, IMPULSE_SIZE, 1, baseband_out);
    fclose(baseband_out);
    return 0;
}
