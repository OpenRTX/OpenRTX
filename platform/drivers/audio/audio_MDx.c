/***************************************************************************
 *   Copyright (C) 2021 by Federico Amedeo Izzo IU2NUO,                    *
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

#include <interfaces/audio.h>
#include <interfaces/gpio.h>
#include <interfaces/delays.h>
#include <hwconfig.h>

void audio_init()
{
    gpio_setMode(SPK_MUTE,     OUTPUT);
    #ifndef PLATFORM_MD9600
    gpio_setMode(AUDIO_AMP_EN, OUTPUT);
    gpio_setMode(MIC_PWR,      OUTPUT);
    #endif

    gpio_setPin(SPK_MUTE);          /* Speaker muted   */
    #ifndef PLATFORM_MD9600
    gpio_clearPin(AUDIO_AMP_EN);    /* Audio PA off    */
    gpio_clearPin(MIC_PWR);         /* Mic preamp. off */
    #endif
}

void audio_terminate()
{
    gpio_setPin(SPK_MUTE);          /* Speaker muted   */
    #ifndef PLATFORM_MD9600
    gpio_clearPin(AUDIO_AMP_EN);    /* Audio PA off    */
    gpio_clearPin(MIC_PWR);         /* Mic preamp. off */
    #endif
}

void audio_enableMic()
{
    #ifndef PLATFORM_MD9600
    gpio_setPin(MIC_PWR);
    #endif
}

void audio_disableMic()
{
    #ifndef PLATFORM_MD9600
    gpio_clearPin(MIC_PWR);
    #endif
}

void audio_enableAmp()
{
    #ifndef PLATFORM_MD9600
    gpio_setPin(AUDIO_AMP_EN);
    #endif
    sleepFor(0, 10);                /* 10ms anti-pop delay */
    gpio_clearPin(SPK_MUTE);
}

void audio_disableAmp()
{
    gpio_setPin(SPK_MUTE);
    #ifndef PLATFORM_MD9600
    gpio_clearPin(AUDIO_AMP_EN);
    #endif
}
