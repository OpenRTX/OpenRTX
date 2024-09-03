/***************************************************************************
 *   Copyright (C) 2020 - 2023 by Federico Amedeo Izzo IU2NUO,             *
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

#include <openrtx.h>


#ifdef PLATFORM_A36PLUS
#include <interfaces/platform.h>
#include <interfaces/delays.h>
#include <peripherals/gpio.h>
#endif

#ifdef PLATFORM_MD9600
#include <interfaces/platform.h>
#include <interfaces/delays.h>
#endif

#ifdef PLATFORM_LINUX
#include <emulator/sdl_engine.h>
#endif

int main(void)
{
    // MD-9600 does not have a proper power on/off mechanism and the MCU is
    // always powered on. We thus need to place a busy wait on the power on
    // button to manage the on/off mechanism.
    // A do-while block is used to avoid re-powering on after a power off due to
    // MCU rebooting before user stops pressing the power button.
    #ifdef PLATFORM_MD9600
    do
    {
        sleepFor(1, 0);
    }
    while(!platform_pwrButtonStatus());
    #endif
    #ifdef PLATFORM_A36PLUS
    if(!platform_pwrButtonStatus())
    {
        sleepFor(0, 50);
        if(!platform_pwrButtonStatus())
        {
            gpio_clearPin(GPIOA, 15);
            delayMs(500);
            NVIC_SystemReset();
            return 0;
        }
    }
    #endif

    openrtx_init();

#ifndef PLATFORM_LINUX
    openrtx_run();
#else
    // macOS requires SDL main loop to run on the main thread.
    // Here we create a new thread for OpenRTX main program and utilize the main
    // thread for the SDL main loop.
    pthread_t openrtx_thread;
    pthread_create(&openrtx_thread, NULL, openrtx_run, NULL);

    sdlEngine_run();
    pthread_join(openrtx_thread, NULL);
#endif
}
