/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <stdint.h>
#include <stdio.h>
#include "interfaces/gpio.h"
#include "interfaces/delays.h"
#include "drivers/tones/toneGenerator_MDx.h"
#include "hwconfig.h"

int main()
{
    gpio_setMode(SPK_MUTE, OUTPUT);     // Turn on speaker
    gpio_clearPin(SPK_MUTE);

    gpio_setMode(AMP_EN, OUTPUT);     // Turn on audio amplifier
    gpio_setPin(AMP_EN);

    toneGen_init();
    toneGen_setBeepFreq(440.0f);

    while(1)
    {
        toneGen_beepOn();
        delayMs(500);
        toneGen_beepOff();
        delayMs(500);
    }

    return 0;
}
