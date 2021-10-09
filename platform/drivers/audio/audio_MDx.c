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

#include <interfaces/audio.h>
#include <interfaces/gpio.h>
#include <interfaces/delays.h>
#include <hwconfig.h>

void audio_init()
{
    gpio_setMode(SPK_MUTE,     OUTPUT);
    #ifndef PLATFORM_MD9600
    gpio_setMode(AUDIO_AMP_EN, OUTPUT);
    #ifndef MDx_ENABLE_SWD
    gpio_setMode(MIC_PWR,      OUTPUT);
    #endif
    #endif

    gpio_setPin(SPK_MUTE);          /* Speaker muted   */
    #ifndef PLATFORM_MD9600
    gpio_clearPin(AUDIO_AMP_EN);    /* Audio PA off    */
    #ifndef MDx_ENABLE_SWD
    gpio_clearPin(MIC_PWR);         /* Mic preamp. off */
    #endif
    #endif
}

void audio_terminate()
{
    gpio_setPin(SPK_MUTE);          /* Speaker muted   */
    #ifndef PLATFORM_MD9600
    gpio_clearPin(AUDIO_AMP_EN);    /* Audio PA off    */
    #ifndef MDx_ENABLE_SWD
    gpio_clearPin(MIC_PWR);         /* Mic preamp. off */
    #endif
    #endif
}

void audio_enableMic()
{
    #if !defined(PLATFORM_MD9600) && !defined(MDx_ENABLE_SWD)
    gpio_setPin(MIC_PWR);
    #endif
}

void audio_disableMic()
{
    #if !defined(PLATFORM_MD9600) && !defined(MDx_ENABLE_SWD)
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
