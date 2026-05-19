/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "calibration/calibInfo_Mod17.h"
#include "interfaces/platform.h"
#include "interfaces/nvmem.h"
#include <stdio.h>
#include "core/gps.h"
#include "emulator.h"

/*
 * Create the data structure holding Module17 calibration data to make the
 * corresponding symbol available to the ui.c object file and, consequently, allow
 * the linker doing its job. This allows to compile and execute OpenRTX on linux
 * with the Module17 UI to make development faster.
 */
mod17Calib_t mod17CalData;


static const hwInfo_t hwInfo =
{
    .vhf_maxFreq = 174,
    .vhf_minFreq = 136,
    .vhf_band    = 1,
    .uhf_maxFreq = 480,
    .uhf_minFreq = 400,
    .uhf_band    = 1,
    .name        = "Linux",
    .hw_version  = 1
};


void platform_init()
{
    nvm_init();
    emulator_start();
}

void platform_terminate()
{
    printf("Platform terminate\n");
    exit(0);
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

datetime_t platform_getCurrentTime()
{
    datetime_t t;

    time_t rawtime;
    struct tm * timeinfo;
    time ( &rawtime );
    // radio expects time to be TZ-less, so use gmtime instead of localtime.
    timeinfo = gmtime ( &rawtime );

    t.hour = timeinfo->tm_hour;
    t.minute = timeinfo->tm_min;
    t.second = timeinfo->tm_sec;
    t.day = timeinfo->tm_wday;
    t.date = timeinfo->tm_mday;
    t.month = timeinfo->tm_mon + 1;
    // Only last two digits of the year are supported in OpenRTX
    t.year = (timeinfo->tm_year + 1900) % 100;

    return t;
}

void platform_setTime(datetime_t t)
{
    (void) t;

    printf("rtc_setTime(t)\n");
}

const hwInfo_t *platform_getHwInfo()
{
    return &hwInfo;
}

const struct gpsDevice *platform_initGps()
{
    extern struct gpsDevice gps;
    return &gps;
}
