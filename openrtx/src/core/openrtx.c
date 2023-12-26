/***************************************************************************
 *   Copyright (C) 2022 - 2023 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN                      *
 *                                Frederik Saraci IU2NRO                   *
 *                                Silvano Seva IU2KWO                      *
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

#include <interfaces/platform.h>
#include <interfaces/keyboard.h>
#include <interfaces/display.h>
#include <interfaces/delays.h>
#include <interfaces/cps_io.h>
#include <peripherals/gps.h>
#include <voicePrompts.h>
#include <graphics.h>
#include <openrtx.h>
#include <threads.h>
#include <state.h>
#include <ui.h>
#ifdef PLATFORM_LINUX
#include <stdlib.h>
#endif

extern void *main_thread(void *arg);

void openrtx_init()
{
    state.devStatus = STARTUP;

    platform_init();    // Initialize low-level platform drivers
    state_init();       // Initialize radio state

    gfx_init();         // Initialize display and graphics driver
    kbd_init();         // Initialize keyboard driver
    ui_init();          // Initialize user interface
    vp_init();          // Initialize voice prompts
    #ifdef CONFIG_SCREEN_CONTRAST
    display_setContrast(state.settings.contrast);
    #endif

    // Load codeplug from nonvolatile memory, create a new one in case of failure.
    if(cps_open(NULL) < 0)
    {
        cps_create(NULL);
        if(cps_open(NULL) < 0)
        {
            // Unrecoverable error
            #ifdef PLATFORM_LINUX
            exit(-1);
            #else
            // TODO: implement error handling for non-linux targets
            while(1) ;
            #endif
        }
    }

    // Display splash screen, turn on backlight after a suitable time to
    // hide random pixels during render process
    ui_drawSplashScreen();
    gfx_render();
    sleepFor(0u, 30u);
    display_setBacklightLevel(state.settings.brightness);

    #if defined(CONFIG_GPS)
    // Detect and initialise GPS
    state.gpsDetected = gps_detect(1000);
    if(state.gpsDetected) gps_init(9600);
    #endif
}

void *openrtx_run()
{
    state.devStatus = RUNNING;

    // Start the OpenRTX threads
    create_threads();

    // Jump to the device management thread
    main_thread(NULL);

    // Device thread terminated, complete shutdown sequence
    state_terminate();
    platform_terminate();

    return NULL;
}
