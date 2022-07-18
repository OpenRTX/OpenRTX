/***************************************************************************
 *   Copyright (C) 2020 - 2022 by Frederik Saraci IU2NRO                   *
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
#include <interfaces/gpio.h>
#include <interfaces/nvmem.h>
#include <stdio.h>
#include "emulator.h"
#include <SDL2/SDL.h>

/* Custom SDL Event to adjust backlight */
extern Uint32 SDL_Backlight_Event;

hwInfo_t hwInfo;

void platform_init()
{
    nvm_init();

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
    exit(0);
}

void platform_setBacklightLevel(uint8_t level)
{
    // Saturate level to 100 and convert value to 0 - 255
    if(level > 100) level = 100;
    uint16_t value = (2 * level) + (level * 55)/100;

    SDL_Event e;
    SDL_zero(e);
    e.type = SDL_Backlight_Event;
    e.user.data1 = malloc(sizeof(uint8_t));
    uint8_t *data = (uint8_t *)e.user.data1;
    *data = ((uint8_t) value);

    SDL_PushEvent(&e);
}

// Simulate a fully charged lithium battery
uint16_t platform_getVbat()
{
    float voltage = emulator_state.vbat;
    if(voltage < 0.0f)  voltage = 0.0f;
    if(voltage > 65.0f) voltage = 65.0f;
    return ((uint16_t) (voltage * 1000.0f));
}

uint8_t platform_getMicLevel()
{
    float level = emulator_state.micLevel;
    if(level < 0.0f)   level = 0.0f;
    if(level > 255.0f) level = 255.0f;

    return ((uint8_t) level);
}

uint8_t platform_getVolumeLevel()
{
    float level = emulator_state.volumeLevel;
    if(level < 0.0f)   level = 0.0f;
    if(level > 255.0f) level = 255.0f;

    return ((uint8_t) level);
}

int8_t platform_getChSelector()
{
    return emulator_state.chSelector;
}

bool platform_getPttStatus()
{
    // Read P key status from SDL
    const uint8_t *state = SDL_GetKeyboardState(NULL);

    if ((state[SDL_SCANCODE_P] != 0) || (emulator_state.PTTstatus == true))
        return true;
    else
        return false;
}

bool platform_pwrButtonStatus()
{
    /* Suppose radio is always on */
    return !emulator_state.powerOff;
}

void platform_ledOn(led_t led)
{
    (void) led;
}

void platform_ledOff(led_t led)
{
    (void) led;
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
