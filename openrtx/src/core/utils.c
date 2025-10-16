/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <string.h>
#include "core/utils.h"
#include <stdio.h>
#include <math.h>
#include "core/cps.h"

uint8_t interpCalParameter(const freq_t freq, const freq_t *calPoints,
                           const uint8_t *param, const uint8_t elems)
{

    if(freq <= calPoints[0])         return param[0];
    if(freq >= calPoints[elems - 1]) return param[elems - 1];

    /* Find calibration point nearest to target frequency */
    uint8_t pos = 0;
    for(; pos < elems; pos++)
    {
        if(calPoints[pos] >= freq) break;
    }

    uint8_t interpValue = 0;
    freq_t  delta = calPoints[pos] - calPoints[pos - 1];

    if(param[pos - 1] < param[pos])
    {
        interpValue = param[pos - 1] + ((freq - calPoints[pos - 1]) *
                                        (param[pos] - param[pos - 1]))/delta;
    }
    else
    {
        interpValue = param[pos - 1] - ((freq - calPoints[pos - 1]) *
                                       (param[pos - 1] - param[pos]))/delta;
    }

    return interpValue;
}

uint32_t bcdToBin(uint32_t bcd)
{
    return ((bcd >> 28) & 0x0F) * 10000000 +
           ((bcd >> 24) & 0x0F) * 1000000 +
           ((bcd >> 20) & 0x0F) * 100000 +
           ((bcd >> 16) & 0x0F) * 10000 +
           ((bcd >> 12) & 0x0F) * 1000 +
           ((bcd >> 8) & 0x0F)  * 100 +
           ((bcd >> 4) & 0x0F)  * 10 +
           (bcd & 0x0F);
}

void stripTrailingZeroes(char *str)
{
    for(size_t i = strlen(str); i > 2; i--)
    {
        if((str[i - 1] != '0') || (str[i - 2] == '.'))
        {
            str[i] = '\0';
            return;
        }
    }
}

uint8_t rssiToSlevel(const rssi_t rssi)
{
    // RSSI >= -53dB is S11 (S9 + 20 dB)
    if(rssi >= -53)
        return 11;

    // RSSI lower than -121dB is always S0
    if(rssi < -121)
        return 0;

    // For S level > 9 use 10dB steps instead of 6dB
    if (rssi >= -73)
        return (uint8_t)((163 + rssi) / 10);

    // For S1 - S9 use 6dB increase per S-Point
    return (uint8_t)(127 + rssi) / 6;
}

uint8_t ctcssFreqToIndex(const uint16_t freq)
{
    for(uint8_t idx = 0; idx < CTCSS_FREQ_NUM; idx += 1)
    {
        if(ctcss_tone[idx] == freq)
            return idx;
    }

    return 255;
}
