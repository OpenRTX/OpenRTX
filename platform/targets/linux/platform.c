/***************************************************************************
 *   Copyright (C) 2020 by Frederik Saraci IU2NRO                          *
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
 *   As a special exception, if other files instantiate templates or use   *
 *   macros or inline functions from this file, or you compile this file   *
 *   and link it with other works to produce a work based on this file,    *
 *   this file does not by itself cause the resulting work to be covered   *
 *   by the GNU General Public License. However the source code for this   *
 *   file must still be made available in accordance with the GNU General  *
 *   Public License. This exception does not invalidate any other reasons  *
 *   why a work based on this file might be covered by the GNU General     *
 *   Public License.                                                       *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#include <interfaces/platform.h>
#include <interfaces/gpio.h>
#include <stdio.h>
#include "emulator.h"
#include <SDL2/SDL.h>

hwInfo_t hwInfo;

void platform_init()
{
    //printf("Platform init\n");
    // Fill hwinfo struct
    memset(&hwInfo, 0x00, sizeof(hwInfo));
    snprintf(hwInfo.name, 10, "Linux");
    // Frequencies are in MHz
    hwInfo.vhf_maxFreq = 174;
    hwInfo.vhf_minFreq = 136;
    hwInfo.vhf_band    = 1;
    hwInfo.uhf_maxFreq = 480;
    hwInfo.uhf_minFreq = 400;
    hwInfo.uhf_band    = 1;

    emulator_start();
}

void platform_terminate()
{
    printf("Platform terminate\n");
}

void platform_setBacklightLevel(__attribute__((unused)) uint8_t level)
{
    //printf("platform_setBacklightLevel(%u)\n", level);
}

// Simulate a fully charged lithium battery
uint16_t platform_getVbat()
{
    float voltage = Radio_State.Vbat;
    if(voltage < 0.0f)  voltage = 0.0f;
    if(voltage > 65.0f) voltage = 65.0f;
    return ((uint16_t) voltage);
}


uint8_t platform_getMicLevel()
{
    float level = Radio_State.micLevel;
    if(level < 0.0f)   level = 0.0f;
    if(level > 255.0f) level = 255.0f;

    return ((uint8_t) level);
}


uint8_t platform_getVolumeLevel()
{
    float level = Radio_State.volumeLevel;
    if(level < 0.0f)   level = 0.0f;
    if(level > 255.0f) level = 255.0f;

    return ((uint8_t) level);
}


int8_t platform_getChSelector()
{
    return Radio_State.chSelector;
}


bool platform_getPttStatus()
{
    // Read P key status from SDL
    SDL_PumpEvents();
    const uint8_t *state = SDL_GetKeyboardState(NULL);
    if (state[SDL_SCANCODE_P])
        return true;
    else
        return false;
}

bool platform_pwrButtonStatus()
{
    /* Suppose radio is always on */
    return true;
}

void platform_ledOn(__attribute__((unused)) led_t led)
{
    // Commented to reduce verbosity on Linux
    //char* str;
    //switch(led)
    //{
    //    case 0:
    //        str = "GREEN";
    //        break;
    //    case 1:
    //        str = "RED";
    //        break;
    //    case 2:
    //        str = "YELLOW";
    //        break;
    //    case 3:
    //        str = "WHITE";
    //        break;
    //}
    //printf("platform_ledOn(%s)\n", str);
}


void platform_ledOff(__attribute__((unused)) led_t led)
{
    // Commented to reduce verbosity on Linux
    //printf("platform_ledOff()\n");
}


void platform_beepStart(uint16_t freq)
{
    printf("platform_beepStart(%u)\n", freq);
}


void platform_beepStop()
{
    printf("platform_beepStop()\n");
}

const void *platform_getCalibrationData()
{
    return NULL;
}

const hwInfo_t *platform_getHwInfo()
{
    return &hwInfo;
}
