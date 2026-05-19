/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <stdio.h>
#include "interfaces/platform.h"
#include "interfaces/delays.h"

int main()
{
    platform_init();

    while(1)
    {
        platform_ledOn(GREEN);
        delayMs(1000);
        platform_ledOff(GREEN);
        delayMs(1000);
    }

    return 0;
}
