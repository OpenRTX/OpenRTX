/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "interfaces/platform.h"
#include "interfaces/keyboard.h"
#include "interfaces/display.h"
#include "interfaces/delays.h"
#include "interfaces/cps_io.h"
#include "core/voicePrompts.h"
#include "core/graphics.h"
#include "core/openrtx.h"
#include "core/threads.h"
#include "core/state.h"
#include "core/ui.h"
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
}

void *openrtx_run(void *arg)
{
    (void) arg;

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
