/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "peripherals/gpio.h"
#include "hwconfig.h"
#include "MAX9814.h"

/*
 * Implementation of MAX9814 gain setting
 */

void max9814_setGain(uint8_t gain)
{
    if (gain == 0)
    {
        gpio_setMode(MIC_GAIN, OUTPUT);
        gpio_setPin(MIC_GAIN); // 40 dB gain
    }
    else if (gain == 1)
    {
        gpio_setMode(MIC_GAIN, OUTPUT);
        gpio_clearPin(MIC_GAIN); // 50 dB gain
    }
    else
    {
        gpio_setMode(MIC_GAIN, INPUT); // High impedance, 60 dB gain
    }
}
