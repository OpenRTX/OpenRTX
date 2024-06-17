/***************************************************************************
 *   Copyright (C) 2024 by Silvano Seva IU2KWO                             *
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
#include <interfaces/delays.h>
#include <peripherals/gpio.h>
#include <calibInfo_Micro17.h>
#include <hwconfig.h>

extern micro17Calib_t micro17CalData;

static const hwInfo_t hwInfo =
{
    .vhf_maxFreq = 0,
    .vhf_minFreq = 0,
    .vhf_band    = 0,
    .uhf_maxFreq = 0,
    .uhf_minFreq = 0,
    .uhf_band    = 0,
    .hw_version  = 0,
    .name        = "Micro17"
};

void platform_init()
{
    gpio_setMode(AIN_HWVER, INPUT_ANALOG);
    gpio_setMode(PTT_IN,    INPUT);
}

void platform_terminate()
{

}

uint16_t platform_getVbat()
{
    return 0;
}

uint8_t platform_getMicLevel()
{
    return 0;
}

uint8_t platform_getVolumeLevel()
{
    return 0;
}

int8_t platform_getChSelector()
{
    return 0;
}

bool platform_getPttStatus()
{
    // Return true if gpio status matches the PTT in active level
    uint8_t ptt_status = gpio_readPin(PTT_IN);
    if(ptt_status == micro17CalData.ptt_in_level)
        return true;

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
    (void) freq;
}

void platform_beepStop()
{

}

const hwInfo_t *platform_getHwInfo()
{
    return &hwInfo;
}
