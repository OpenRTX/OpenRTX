/***************************************************************************
 *   Copyright (C) 2020 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
 *                         Frederik Saraci IU2NRO                          *
 *                         Silvano Seva IU2KWO                             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#include <ui.h>
#include <stdlib.h>
#include <inttypes.h>
#include <threads.h>
#include <interfaces/platform.h>
#include <battery.h>
#include <interfaces/graphics.h>
#include <interfaces/delays.h>
#include <hwconfig.h>

int main(void)
{
    // Initialize platform drivers
    platform_init();

    // Initialize graphics driver
    gfx_init();

    // Initialize user interface
    ui_init();

    // Display splash screen
    ui_drawSplashScreen(true);
    gfx_render();

    // Wait 30ms before turning on backlight to hide random pixels on screen
    sleepFor(0u, 30u);
    platform_setBacklightLevel(255);

    // Keep the splash screen for 1 second
    sleepFor(1u, 0u);

    // Create OpenRTX threads
    create_threads();

    // Auxiliary functions loop
    while(true)
    {
        // No low-frequency function at the moment
        ;
    }
}
