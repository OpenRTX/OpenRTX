/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "core/graphics.h"
#include "interfaces/platform.h"
#include "interfaces/display.h"
#include "interfaces/delays.h"

int main()
{
    platform_init();
    display_init();
    gfx_init();

    display_setBacklightLevel(255);

    while(1)
    {
        point_t start = {0, 0};
        #ifdef CONFIG_PIX_FMT_BW
        color_t band1 = {  0,   0,   0, 255};
        color_t band2 = {255, 255, 255, 255};
        color_t band3 = {  0,   0,   0, 255};
        color_t band4 = {255, 255, 255, 255};
        #else
        color_t band1 = {255,   0,   0, 255};    // Red
        color_t band2 = {  0  255,   0, 255};    // Green
        color_t band3 = {  0,   0, 255, 255};    // Blue
        color_t band4 = {255, 255, 255, 255};    // White
        #endif

        gfx_clearScreen();
        gfx_drawRect(start, CONFIG_SCREEN_WIDTH/4, CONFIG_SCREEN_HEIGHT, band1, true);

        start.x += CONFIG_SCREEN_WIDTH/4;
        gfx_drawRect(start, CONFIG_SCREEN_WIDTH/4, CONFIG_SCREEN_HEIGHT, band2, true);

        start.x += CONFIG_SCREEN_WIDTH/4;
        gfx_drawRect(start, CONFIG_SCREEN_WIDTH/4, CONFIG_SCREEN_HEIGHT, band3, true);

        start.x += CONFIG_SCREEN_WIDTH/4;
        gfx_drawRect(start, CONFIG_SCREEN_WIDTH/4, CONFIG_SCREEN_HEIGHT, band4, true);

        platform_ledOn(GREEN);
        gfx_render();
        platform_ledOff(GREEN);
        sleepFor(0, 50);
    }

    return 0;
}

