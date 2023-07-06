/***************************************************************************
 *   Copyright (C) 2020 - 2023 by Frederik Saraci IU2NRO                   *
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
#include <stdlib.h>
#include <string.h>

hwInfo_t hwInfo;

void platform_init()
{
    // Fill hwinfo struct
    memset(&hwInfo, 0x00, sizeof(hwInfo));
    snprintf(hwInfo.name, 10, "Zephyr");

    // TODO: Read frequencies from devicetree?
    // Frequencies are in MHz
    hwInfo.vhf_maxFreq = 174;
    hwInfo.vhf_minFreq = 136;
    hwInfo.vhf_band    = 1;
    hwInfo.uhf_maxFreq = 480;
    hwInfo.uhf_minFreq = 400;
    hwInfo.uhf_band    = 1;
}

void platform_terminate()
{
    printf("Platform terminate\n");
    exit(0);
}

void platform_setBacklightLevel(uint8_t level)
{
    // TODO: Implement
    ;
}

// Simulate a fully charged lithium battery
uint16_t platform_getVbat()
{
    // TODO: implement voltage readout
    float voltage = 0.0f;
    if(voltage < 0.0f)  voltage = 0.0f;
    if(voltage > 65.0f) voltage = 65.0f;
    return ((uint16_t) (voltage * 1000.0f));
}

uint8_t platform_getMicLevel()
{
    return 128;
}

uint8_t platform_getVolumeLevel()
{
    return 128;
}

int8_t platform_getChSelector()
{
    return 0;
}

bool platform_getPttStatus()
{
    return false;
}

bool platform_pwrButtonStatus()
{
    return true;
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
