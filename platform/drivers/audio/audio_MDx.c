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
    gpio_setMode(AUDIO_AMP_EN, OUTPUT);
    gpio_setMode(MIC_PWR,      OUTPUT);

    gpio_setPin(SPK_MUTE);          /* Speaker muted   */
    gpio_clearPin(AUDIO_AMP_EN);    /* Audio PA off    */
    gpio_clearPin(MIC_PWR);         /* Mic preamp. off */
}

void audio_terminate()
{
    gpio_setPin(SPK_MUTE);          /* Speaker muted   */
    gpio_clearPin(AUDIO_AMP_EN);    /* Audio PA off    */
    gpio_clearPin(MIC_PWR);         /* Mic preamp. off */
}

void audio_enableMic()
{
    gpio_setPin(MIC_PWR);
}

void audio_disableMic()
{
    gpio_clearPin(MIC_PWR);
}

void audio_enableAmp()
{
    gpio_setPin(AUDIO_AMP_EN);
    sleepFor(0, 10);                /* 10ms anti-pop delay */
    gpio_clearPin(SPK_MUTE);
}

void audio_disableAmp()
{
    gpio_setPin(SPK_MUTE);
    gpio_clearPin(AUDIO_AMP_EN);
}
