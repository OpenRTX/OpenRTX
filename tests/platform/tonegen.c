/***************************************************************************
 *   Copyright (C) 2020 by Federico Izzo IU2NUO, Niccolò Izzo IU2KIN and   *
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

#include <stdint.h>
#include <stdio.h>

#include "hwconfig.h"
#include "interfaces/delays.h"
#include "interfaces/gpio.h"
#include "platform/drivers/tones/toneGenerator_MDx.h"

int main()
{
    gpio_setMode(SPK_MUTE, OUTPUT);     // Turn on speaker
    gpio_clearPin(SPK_MUTE);

    gpio_setMode(AMP_EN, OUTPUT);     // Turn on audio amplifier
    gpio_setPin(AMP_EN);

    toneGen_init();
    toneGen_setBeepFreq(440.0f);

    while(1)
    {
        toneGen_beepOn();
        delayMs(500);
        toneGen_beepOff();
        delayMs(500);
    }

    return 0;
}
